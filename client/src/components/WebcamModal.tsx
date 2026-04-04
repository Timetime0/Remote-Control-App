import { useModalEscape } from '../hooks/useModalEscape';

type WebcamModalProps = {
  open: boolean;
  targetLabel: string;
  busy: boolean;
  onClose: () => void;
  onStartWebcam: () => void;
  onStartRecord: () => void;
  onStopRecord: () => void;
};

function WebcamModal({
  open,
  targetLabel,
  busy,
  onClose,
  onStartWebcam,
  onStartRecord,
  onStopRecord,
}: WebcamModalProps) {
  useModalEscape(open, onClose);
  if (!open) return null;

  return (
    <div className="modal-backdrop" onClick={onClose} role="presentation">
      <div className="modal-panel process-modal" onClick={(e) => e.stopPropagation()} role="dialog">
        <div className="modal-header">
          <div>
            <h2>Webcam</h2>
            <p className="modal-sub">{targetLabel}</p>
          </div>
          <button className="btn modal-close" onClick={onClose} type="button">
            Đóng
          </button>
        </div>
        <div className="process-modal-toolbar">
          <button className="btn primary" disabled={busy} onClick={onStartWebcam} type="button">
            Start webcam
          </button>
          <button className="btn" disabled={busy} onClick={onStartRecord} type="button">
            Start record
          </button>
          <button className="btn btn-danger" disabled={busy} onClick={onStopRecord} type="button">
            Stop record
          </button>
        </div>
      </div>
    </div>
  );
}

export default WebcamModal;
