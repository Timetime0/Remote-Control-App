import { useState } from 'react';
import { useModalEscape } from '../hooks/useModalEscape';

type MouseControlModalProps = {
  open: boolean;
  busy: boolean;
  targetLabel: string;
  onClose: () => void;
  onMove: (x: number, y: number) => void;
  onLeftClick: (x: number, y: number) => void;
  onRightClick: (x: number, y: number) => void;
};

function MouseControlModal({
  open,
  busy,
  targetLabel,
  onClose,
  onMove,
  onLeftClick,
  onRightClick,
}: MouseControlModalProps) {
  useModalEscape(open, onClose);

  const [x, setX] = useState('500');
  const [y, setY] = useState('300');

  if (!open) return null;

  const px = Number(x);
  const py = Number(y);

  return (
    <div className="modal-backdrop" onClick={onClose} role="presentation">
      <div className="modal-panel process-modal" onClick={(e) => e.stopPropagation()} role="dialog">
        <div className="modal-header">
          <div>
            <h2>Mouse Control</h2>
            <p className="modal-sub">{targetLabel}</p>
          </div>

          <button className="btn modal-close" onClick={onClose} type="button">
            Close
          </button>
        </div>

        <div className="process-modal-toolbar">
          <input
            className="process-start-input"
            onChange={(e) => setX(e.target.value)}
            placeholder="X"
            type="number"
            value={x}
          />

          <input
            className="process-start-input"
            onChange={(e) => setY(e.target.value)}
            placeholder="Y"
            type="number"
            value={y}
          />

          <button
            className="btn"
            disabled={busy}
            onClick={() => onMove(px, py)}
            type="button"
          >
            Move
          </button>

          <button
            className="btn primary"
            disabled={busy}
            onClick={() => onLeftClick(px, py)}
            type="button"
          >
            Left Click
          </button>

          <button
            className="btn btn-danger"
            disabled={busy}
            onClick={() => onRightClick(px, py)}
            type="button"
          >
            Right Click
          </button>
        </div>
      </div>
    </div>
  );
}

export default MouseControlModal;