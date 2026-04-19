import { useModalEscape } from '../hooks/useModalEscape';

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
    onClear: () => void;
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
    onClear,
}: KeyloggerModalProps) {
    useModalEscape(open, onClose);

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
                            {targetLabel} · agent {running ? 'đang ghi' : 'đã dừng'}
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
                        onClick={onStop}
                        type="button"
                    >
                        Stop
                    </button>

                    <button className="btn" disabled={busy} onClick={onRefresh} type="button">
                        Refresh
                    </button>

                    <button className="btn" disabled={busy} onClick={onClear} type="button">
                        Clear view
                    </button>
                </div>

                <div className="process-table-wrap">
                    <p className="modal-sub keylogger-log-hint">
                        Log do agent ghi trên máy đích (Windows: mã phím ảo vk=…; macOS: keycode).
                    </p>

                    <pre
                        className="modal-body-inline"
                        style={{
                            whiteSpace: 'pre-wrap',
                            wordBreak: 'break-word',
                        }}
                    >
                        {content || '(chưa có dữ liệu — bấm Start trên agent rồi Refresh / đợi tự cập nhật)'}
                    </pre>
                </div>
            </div>
        </div>
    );
}

export default KeyloggerModal;
