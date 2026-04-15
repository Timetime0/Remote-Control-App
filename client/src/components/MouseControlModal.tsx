import { useRef, useState } from 'react';
import { useModalEscape } from '../hooks/useModalEscape';

type MousePayload = {
  x: number;
  y: number;
  viewerWidth: number;
  viewerHeight: number;
  remoteWidth: number;
  remoteHeight: number;
};

type MouseControlModalProps = {
  open: boolean;
  busy: boolean;
  imageUrl: string;
  targetLabel: string;
  onClose: () => void;
  onMouseMove: (payload: MousePayload) => void;
  onLeftClick: (payload: MousePayload) => void;
  onRightClick: (payload: MousePayload) => void;
};

function MouseControlModal({
  open,
  busy,
  imageUrl,
  targetLabel,
  onClose,
  onMouseMove,
  onLeftClick,
  onRightClick,
}: MouseControlModalProps) {
  const imageRef = useRef<HTMLImageElement | null>(null);
  const lastMoveRef = useRef(0);
  const [cursor, setCursor] = useState<{ x: number; y: number } | null>(null);

  useModalEscape(open, onClose);

  if (!open) return null;

  const buildPayload = (
    event: React.MouseEvent<HTMLImageElement>,
  ): MousePayload | null => {
    const image = imageRef.current;
    if (!image) return null;

    const rect = image.getBoundingClientRect();

    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;

    if (x < 0 || y < 0 || x > rect.width || y > rect.height) {
      return null;
    }

    return {
      x: Math.round(x),
      y: Math.round(y),
      viewerWidth: Math.round(rect.width),
      viewerHeight: Math.round(rect.height),
      remoteWidth: 1920,
      remoteHeight: 1080,
    };
  };

  return (
    <div className="modal-backdrop" onClick={onClose} role="presentation">
      <div
        className="modal-panel mouse-viewer-modal"
        onClick={(e) => e.stopPropagation()}
        role="dialog"
      >
        <div className="modal-header">
          <div>
            <h2>Mouse Control</h2>
            <p className="modal-sub">{targetLabel}</p>
          </div>

          <button className="btn modal-close" onClick={onClose} type="button">
            Close
          </button>
        </div>

        <div className="mouse-viewer-body">
        {imageUrl ? (
  <div className="mouse-viewer-image-wrap">
                <img
                ref={imageRef}
                alt="Mouse control preview"
                className="mouse-viewer-image"
                draggable={false}
                src={imageUrl}
                onClick={(e) => {
                  const payload = buildPayload(e);
                  if (payload) {
                    onLeftClick(payload);
                  }
                }}
                onContextMenu={(e) => {
                  e.preventDefault();

                  const payload = buildPayload(e);
                  if (payload) {
                    onRightClick(payload);
                  }
                }}
                onMouseMove={(e) => {
                  const now = Date.now();

                  if (now - lastMoveRef.current < 30) {
                    return;
                  }

                  lastMoveRef.current = now;

                  const payload = buildPayload(e);

                  if (payload) {
                    setCursor({
                      x: payload.x,
                      y: payload.y,
                    });

                    onMouseMove(payload);
                  }
                }}
              />

              {cursor && (
                <div
                  className="mouse-viewer-cursor"
                  style={{
                    left: cursor.x,
                    top: cursor.y,
                  }}
                />
              )}
            </div>
          ) : (
            <div className="mouse-viewer-empty">
              Waiting for screen preview...
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

export default MouseControlModal;