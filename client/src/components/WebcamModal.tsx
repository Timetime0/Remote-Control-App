import { useEffect, useState } from 'react';
import { useModalEscape } from '../hooks/useModalEscape';

type WebcamModalProps = {
  open: boolean;
  pcId: string | null;
  targetLabel: string;
  busy: boolean;
  onClose: () => void;
};

function WebcamModal({ open, pcId, targetLabel, busy, onClose }: WebcamModalProps) {
  useModalEscape(open, onClose);

  const [status, setStatus] = useState('Idle');
  const [previewUrl, setPreviewUrl] = useState('');
  const [hasStream, setHasStream] = useState(false);
  const [recording, setRecording] = useState(false);
  const [actionBusy, setActionBusy] = useState(false);

  const combinedBusy = busy || actionBusy;

  useEffect(() => {
    if (!open || !pcId) return;

    const unsub = window.remoteApi.onWebcamFrame((payload) => {
      if (payload.command === 'WEBCAM_START') {
        if (payload.ok === false) {
          setStatus(typeof payload.message === 'string' ? payload.message : 'Webcam start failed');
          setHasStream(false);
          setPreviewUrl('');
        } else {
          setStatus('Camera started');
        }
        return;
      }

      if (
        payload.ok === true &&
        payload.command === 'WEBCAM_FRAME' &&
        typeof payload.mime === 'string' &&
        typeof payload.data === 'string'
      ) {
        setPreviewUrl(`data:${payload.mime};base64,${payload.data}`);
        setHasStream(true);
      } else if (payload.ok === false && typeof payload.message === 'string') {
        setStatus(payload.message);
      }
    });

    return () => {
      unsub();
      void window.remoteApi.stopWebcam(pcId);
    };
  }, [open, pcId]);

  useEffect(() => {
    if (!open) {
      setStatus('Idle');
      setPreviewUrl('');
      setHasStream(false);
      setRecording(false);
      setActionBusy(false);
    }
  }, [open]);

  const handleClose = () => {
    if (pcId) {
      void window.remoteApi.stopWebcam(pcId);
    }
    onClose();
  };

  const handleStartWebcam = async () => {
    if (!pcId) {
      alert('Chưa chọn máy đích.');
      return;
    }
    if (hasStream) {
      setStatus('Webcam đã bật');
      return;
    }

    setActionBusy(true);
    setStatus('Connecting…');
    setPreviewUrl('');
    try {
      const res = await window.remoteApi.startWebcam(pcId);
      if (!res.ok) {
        setStatus(res.message ?? 'Cannot start webcam stream');
      } else {
        setStatus('Waiting for agent…');
      }
    } catch (e) {
      setStatus((e as Error).message);
    } finally {
      setActionBusy(false);
    }
  };

  const handleStartRecord = async () => {
    if (!pcId) return;
    if (!hasStream) {
      alert('Hãy bật webcam trước.');
      return;
    }

    setActionBusy(true);
    setStatus('Starting record on agent…');
    try {
      const result = await window.remoteApi.runCommand(pcId, 'WEBCAM_RECORD_START');
      const parsed = JSON.parse(result.raw) as { ok?: boolean; message?: string };
      if (parsed.ok === true) {
        setRecording(true);
        setStatus('Recording (on remote PC)…');
      } else {
        setStatus(parsed.message ?? 'WEBCAM_RECORD_START failed');
        alert(parsed.message ?? 'Không bắt đầu ghi trên agent được.');
      }
    } catch (e) {
      setStatus((e as Error).message);
    } finally {
      setActionBusy(false);
    }
  };

  const handleStopRecord = async () => {
    if (!pcId) return;

    setActionBusy(true);
    setStatus('Stopping record & preparing file…');
    try {
      const result = await window.remoteApi.runCommand(pcId, 'WEBCAM_RECORD_STOP');
      const parsed = JSON.parse(result.raw) as {
        ok?: boolean;
        path?: string;
        filename?: string;
        message?: string;
      };

      if (parsed.ok !== true || typeof parsed.path !== 'string') {
        setStatus(parsed.message ?? 'WEBCAM_RECORD_STOP failed');
        alert(parsed.message ?? 'Dừng ghi trên agent thất bại.');
        return;
      }

      setRecording(false);

      const defaultName =
        typeof parsed.filename === 'string' && parsed.filename.length > 0
          ? parsed.filename
          : 'webcam-record.avi';

      const localPath = await window.remoteApi.pickSaveFile(defaultName);
      if (!localPath) {
        setStatus('Recording ready on agent (save cancelled)');
        return;
      }

      setStatus('Downloading recording…');
      const dl = await window.remoteApi.downloadFile(pcId, parsed.path, localPath);
      if (dl.ok) {
        setStatus(`Saved: ${localPath}`);
      } else {
        setStatus(dl.message);
        alert(`Tải file về máy client thất bại: ${dl.message}`);
      }
    } catch (e) {
      setStatus((e as Error).message);
      alert((e as Error).message);
    } finally {
      setActionBusy(false);
    }
  };

  const handleStopWebcam = async () => {
    if (!pcId) return;

    setActionBusy(true);
    setStatus('Stopping webcam on agent…');
    try {
      await window.remoteApi.stopWebcam(pcId);
      setPreviewUrl('');
      setHasStream(false);
      setRecording(false);
      setStatus('Camera stopped');
    } catch (e) {
      setStatus((e as Error).message);
    } finally {
      setActionBusy(false);
    }
  };

  if (!open) return null;

  return (
    <div className="modal-backdrop" onClick={handleClose} role="presentation">
      <div
        className="modal-panel process-modal"
        onClick={(e) => e.stopPropagation()}
        role="dialog"
        aria-modal="true"
        aria-labelledby="webcam-title"
      >
        <div className="modal-header">
          <div>
            <h2 id="webcam-title">Webcam (remote agent)</h2>
            <p className="modal-sub">{targetLabel}</p>
            <p className="modal-sub">Status: {status}</p>
          </div>

          <button className="btn modal-close" onClick={handleClose} type="button">
            Close
          </button>
        </div>

        <div className="process-modal-toolbar">
          <button
            className="btn primary"
            disabled={combinedBusy || hasStream}
            onClick={() => void handleStartWebcam()}
            type="button"
          >
            Start webcam
          </button>

          <button
            className="btn"
            disabled={combinedBusy || !hasStream || recording}
            onClick={() => void handleStartRecord()}
            type="button"
          >
            Start record
          </button>

          <button
            className="btn btn-danger"
            disabled={combinedBusy || !recording}
            onClick={() => void handleStopRecord()}
            type="button"
          >
            Stop record
          </button>

          <button
            className="btn"
            disabled={combinedBusy || !hasStream}
            onClick={() => void handleStopWebcam()}
            type="button"
          >
            Stop webcam
          </button>
        </div>

        <div style={{ marginTop: 16 }}>
          {previewUrl ? (
            <img
              alt="Remote webcam"
              src={previewUrl}
              style={{
                width: '100%',
                maxHeight: 420,
                borderRadius: 10,
                background: '#000',
                objectFit: 'contain',
              }}
            />
          ) : (
            <div
              style={{
                width: '100%',
                height: 320,
                borderRadius: 10,
                background: '#000',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                color: '#888',
              }}
            >
              No video yet — start webcam on the agent
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

export default WebcamModal;
