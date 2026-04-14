import { useRef, useState } from 'react';
import { useModalEscape } from '../hooks/useModalEscape';

type KeyloggerModalProps = {
  open: boolean;
  pcId: string | null;
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
  pcId,
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
  const inputRef = useRef<HTMLTextAreaElement | null>(null);
  const [inputValue, setInputValue] = useState('');

  useModalEscape(open, onClose);

  if (!open) return null;

  const handleStart = () => {
    setInputValue('');
    onStart();
    setTimeout(() => {
      inputRef.current?.focus();
    }, 50);
  };

  const handleClear = () => {
    setInputValue('');
    onClear();
    setTimeout(() => {
      inputRef.current?.focus();
    }, 50);
  };

  const handleInputChange = async (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    const newValue = e.target.value;

    if (!pcId || !running) {
      setInputValue(newValue);
      return;
    }

    try {
      if (newValue.length > inputValue.length) {
        const addedText = newValue.slice(inputValue.length);

        for (const ch of addedText) {
          let key = ch;

          if (ch === '\n') key = '[Enter]';
          else if (ch === ' ') key = '[Space]';

          await window.remoteApi.runCommand(pcId, `KEYLOGGER_ADD ${key}`);
        }
      } else if (newValue.length < inputValue.length) {
        const removedCount = inputValue.length - newValue.length;

        for (let i = 0; i < removedCount; i += 1) {
          await window.remoteApi.runCommand(pcId, 'KEYLOGGER_ADD [Backspace]');
        }
      }

      setInputValue(newValue);
    } catch (error) {
      console.error('KEYLOGGER_ADD failed', error);
    }
  };

  return (
    <div className="modal-backdrop" onClick={onClose} role="presentation">
      <div
        className="modal-panel process-modal"
        onClick={(e) => e.stopPropagation()}
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
            onClick={handleStart}
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

          <button
            className="btn"
            disabled={busy}
            onClick={onRefresh}
            type="button"
          >
            Refresh
          </button>

          <button
            className="btn"
            disabled={busy}
            onClick={handleClear}
            type="button"
          >
            Clear
          </button>
        </div>

        <div style={{ marginBottom: 12 }}>
          <p className="modal-sub" style={{ marginBottom: 6 }}>
            Ô input test: bấm Start rồi gõ vào đây để kiểm tra.
          </p>

          <textarea
            ref={inputRef}
            placeholder="Gõ vào đây để test keylogger..."
            value={inputValue}
            onChange={handleInputChange}
            style={{
              width: '100%',
              minHeight: 100,
              padding: 10,
              borderRadius: 8,
              border: '1px solid #3a4664',
              background: '#0f172a',
              color: '#e5eefc',
              resize: 'vertical',
              outline: 'none',
              boxSizing: 'border-box',
            }}
          />
        </div>

        <div className="process-table-wrap">
          <p className="modal-sub keylogger-log-hint">
            Chỉ ghi phím trong ô test của app khi đang bật.
          </p>

          <pre
            className="modal-body-inline"
            style={{
              whiteSpace: 'pre-wrap',
              wordBreak: 'break-word',
            }}
          >
            {content || '(empty log)'}
          </pre>
        </div>
      </div>
    </div>
  );
}

export default KeyloggerModal;