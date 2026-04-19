import { useEffect } from 'react';
import { useModalEscape } from '../hooks/useModalEscape';

type KeyloggerModalProps = {
    open: boolean;
    targetLabel: string;
    running: boolean;
    content: string;
    busy: boolean;
    onClose: () => void;
    onStart: () => void;
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
    onRefresh,
}: KeyloggerModalProps) {
    useModalEscape(open, onClose);

    useEffect(() => {
        // no-op
    }, [open, running]);

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
                        disabled={busy || !running}
                        onClick={onRefresh}
                        type="button"
                    >
                        Refresh
                    </button>
                </div>

                <div className="process-table-wrap">
                    <p className="modal-sub keylogger-log-hint">
                        Nội dung gõ trên máy server sẽ hiển thị bên dưới.
                    </p>

                    <pre
                        className="modal-body-inline"
                        style={{
                            whiteSpace: 'pre-wrap',
                            wordBreak: 'break-word',
                            minHeight: 220,
                        }}
                    >
                        {content || '(waiting for keyboard activity...)'}
                    </pre>
                </div>
            </div>
        </div>
    );
}

export default KeyloggerModal;