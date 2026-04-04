import { useState } from 'react';
import { useModalEscape } from '../hooks/useModalEscape';

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
  onLocalPathChange: (v: string) => void;
  onRemotePathChange: (v: string) => void;
  onRemoteDirChange: (v: string) => void;
  onPickLocalFile: () => void;
  onUpload: () => void;
  onPickSavePath: () => void;
  onLoadRemoteFiles: () => void;
  onSelectRemoteFile: (v: string) => void;
  onDownload: () => void;
};

function FileTransferModal(props: FileTransferModalProps) {
  const [tab, setTab] = useState<'upload' | 'download'>('upload');
  useModalEscape(props.open, props.onClose);
  if (!props.open) return null;

  return (
    <div className="modal-backdrop" onClick={props.onClose} role="presentation">
      <div className="modal-panel process-modal" onClick={(e) => e.stopPropagation()} role="dialog">
        <div className="modal-header">
          <div>
            <h2>File Transfer</h2>
            <p className="modal-sub">{props.targetLabel}</p>
          </div>
          <button className="btn modal-close" onClick={props.onClose} type="button">
            Đóng
          </button>
        </div>
        <div className="process-modal-toolbar">
          <button
            className={`btn ${tab === 'upload' ? 'primary' : ''}`}
            disabled={props.busy}
            onClick={() => setTab('upload')}
            type="button"
          >
            Upload to server
          </button>
          <button
            className={`btn ${tab === 'download' ? 'primary' : ''}`}
            disabled={props.busy}
            onClick={() => setTab('download')}
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
              Remote path
              <input
                className="process-start-input"
                onChange={(e) => props.onRemotePathChange(e.target.value)}
                type="text"
                value={props.remotePath}
              />
            </label>
            <button
              className="btn primary"
              disabled={props.busy}
              onClick={props.onUpload}
              type="button"
            >
              Upload
            </button>
          </div>
        ) : (
          <div className="process-table-wrap transfer-form">
            <label>
              Remote directory
              <div className="transfer-row">
                <input
                  className="process-start-input"
                  onChange={(e) => props.onRemoteDirChange(e.target.value)}
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
              onClick={props.onDownload}
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
