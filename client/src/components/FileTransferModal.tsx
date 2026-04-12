import { useEffect, useState } from 'react';
import { useModalEscape } from '../hooks/useModalEscape';
import type { CommandResult } from '../types';

type FileTransferModalProps = {
  open: boolean;
  targetLabel: string;
  localPath: string;
  remotePath: string;
  remoteDir: string;
  remoteFiles: string[];
  selectedRemoteFile: string | null;
  busy: boolean;
  onClose: () => void;
  /** Clears path fields when switching between Upload and Download tabs. */
  onResetPaths: () => void;
  onLocalPathChange: (v: string) => void;
  onRemotePathChange: (v: string) => void;
  onRemoteDirChange: (v: string) => void;
  onPickLocalFile: () => void;
  onUpload: () => Promise<CommandResult>;
  onPickSavePath: () => void;
  onLoadRemoteFiles: () => void;
  onSelectRemoteFile: (v: string) => void;
  onDownload: () => Promise<CommandResult>;
};

function clipForAlert(s: string, max = 600): string {
  const t = s.trim();
  if (t.length <= max) return t;
  return `${t.slice(0, max)}…`;
}

function showTransferAlert(title: string, result: CommandResult) {
  const extra = result.raw ? `\n\n${clipForAlert(result.raw)}` : '';
  window.alert(`${title}\n\n${result.message}${extra}`);
}

function FileTransferModal(props: FileTransferModalProps) {
  const [tab, setTab] = useState<'upload' | 'download'>('upload');
  useModalEscape(props.open, props.onClose);
  useEffect(() => {
    if (props.open) setTab('upload');
  }, [props.open]);
  if (!props.open) return null;

  const handleUpload = async () => {
    try {
      const result = await props.onUpload();
      showTransferAlert(result.ok ? 'Upload succeeded' : 'Upload failed', result);
      if (result.ok) props.onClose();
    } catch (e) {
      window.alert(`Upload error\n\n${(e as Error).message}`);
    }
  };

  const handleDownload = async () => {
    try {
      const result = await props.onDownload();
      showTransferAlert(result.ok ? 'Download succeeded' : 'Download failed', result);
      if (result.ok) props.onClose();
    } catch (e) {
      window.alert(`Download error\n\n${(e as Error).message}`);
    }
  };

  return (
    <div className="modal-backdrop" onClick={props.onClose} role="presentation">
      <div className="modal-panel process-modal" onClick={(e) => e.stopPropagation()} role="dialog">
        <div className="modal-header">
          <div>
            <h2>File Transfer</h2>
            <p className="modal-sub">{props.targetLabel}</p>
          </div>
          <button className="btn modal-close" onClick={props.onClose} type="button">
            Close
          </button>
        </div>
        <div className="process-modal-toolbar">
          <button
            className={`btn ${tab === 'upload' ? 'primary' : ''}`}
            disabled={props.busy}
            onClick={() => {
              if (tab !== 'upload') props.onResetPaths();
              setTab('upload');
            }}
            type="button"
          >
            Upload to server
          </button>
          <button
            className={`btn ${tab === 'download' ? 'primary' : ''}`}
            disabled={props.busy}
            onClick={() => {
              if (tab !== 'download') props.onResetPaths();
              setTab('download');
            }}
            type="button"
          >
            Download from server
          </button>
        </div>

        {tab === 'upload' ? (
          <div className="process-table-wrap transfer-form">
            <label>
              Local file
              <div className="transfer-row">
                <input
                  className="process-start-input"
                  onChange={(e) => props.onLocalPathChange(e.target.value)}
                  type="text"
                  value={props.localPath}
                />
                <button
                  className="btn"
                  disabled={props.busy}
                  onClick={props.onPickLocalFile}
                  type="button"
                >
                  Browse
                </button>
              </div>
            </label>
            <label>
              Remote file name (the file is saved in the desktop directory)
              <input
                className="process-start-input"
                onChange={(e) => props.onRemotePathChange(e.target.value)}
                placeholder="e.g. report or report.pdf — extension copied from local file if omitted"
                type="text"
                value={props.remotePath}
              />
            </label>
            <button
              className="btn primary"
              disabled={props.busy}
              onClick={() => void handleUpload()}
              type="button"
            >
              Upload
            </button>
          </div>
        ) : (
          <div className="process-table-wrap transfer-form">
            <label>
              Remote directory (macOS: ~ = home, ~/Desktop = Desktop)
              <div className="transfer-row">
                <input
                  className="process-start-input"
                  onChange={(e) => props.onRemoteDirChange(e.target.value)}
                  placeholder="~  or  ~/Desktop  or  /full/path"
                  type="text"
                  value={props.remoteDir}
                />
                <button
                  className="btn"
                  disabled={props.busy}
                  onClick={props.onLoadRemoteFiles}
                  type="button"
                >
                  Load files
                </button>
              </div>
            </label>

            <div className="remote-file-list">
              {props.remoteFiles.map((file) => (
                <button
                  className={`btn remote-file-item ${props.selectedRemoteFile === file ? 'primary' : ''}`}
                  key={file}
                  onClick={() => props.onSelectRemoteFile(file)}
                  type="button"
                >
                  {file}
                </button>
              ))}
            </div>

            <label>
              Save to local path
              <div className="transfer-row">
                <input
                  className="process-start-input"
                  onChange={(e) => props.onLocalPathChange(e.target.value)}
                  type="text"
                  value={props.localPath}
                />
                <button
                  className="btn"
                  disabled={props.busy}
                  onClick={props.onPickSavePath}
                  type="button"
                >
                  Browse
                </button>
              </div>
            </label>
            <button
              className="btn primary"
              disabled={props.busy}
              onClick={() => void handleDownload()}
              type="button"
            >
              Download
            </button>
          </div>
        )}
      </div>
    </div>
  );
}

export default FileTransferModal;
