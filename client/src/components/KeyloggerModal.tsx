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
  onClose: () => void;
  onStart: () => void;
  onStop: () => void;
  onClearLog: () => void;
};

function KeyloggerModal({
  open,
  targetLabel,
  targetOs,
  running,
  content,
  busy,
  onClose,
  onStart,
  onStop,
  onClearLog,
}: KeyloggerModalProps) {
  useModalEscape(open, onClose);

  const decodedText = useMemo(() => formatKeylogDisplay(content, targetOs), [content, targetOs]);

  const logBody = useMemo(() => {
    if (decodedText.length > 0) {
      return decodedText;
    }
    if (content.trim()) {
      return content;
    }
    return '(waiting for keyboard activity...)';
  }, [content, decodedText]);

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
                        <p className="modal-sub">{targetLabel} · status {running ? 'Running' : 'Stopped'}</p>
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
                        className="btn"
                        disabled={busy || !running}
                        onClick={onStop}
                        type="button"
                    >
                        Stop
                    </button>

                    <button
                        className="btn btn-danger"
                        disabled={busy}
                        onClick={onClearLog}
                        type="button"
                    >
                        Clear logs
                    </button>
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