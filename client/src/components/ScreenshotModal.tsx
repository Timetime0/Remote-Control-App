import { useState } from 'react';
import ReactCrop, { type Crop } from 'react-image-crop';
import 'react-image-crop/dist/ReactCrop.css';
import { useModalEscape } from '../hooks/useModalEscape';

type ScreenshotModalProps = {
  open: boolean;
  onClose: () => void;
  targetLabel: string;
  fetchedAt: number;
  imageUrl: string;
};

function ScreenshotModal({
  open,
  onClose,
  targetLabel,
  fetchedAt,
  imageUrl,
}: ScreenshotModalProps) {
  useModalEscape(open, onClose);

  const [crop, setCrop] = useState<Crop>({
    unit: '%',
    x: 0,
    y: 0,
    width: 100,
    height: 100,
  });

  if (!open) return null;

  const filename = `remote-screenshot-${Date.now()}.png`;

  return (
    <div className="modal-backdrop" onClick={onClose} role="presentation">
      <div
        className="modal-panel screenshot-modal"
        onClick={(e) => e.stopPropagation()}
        role="dialog"
      >
        <div className="modal-header">
          <div>
            <h2>Screenshot</h2>
            <p className="modal-sub">
              {targetLabel} · fetched {new Date(fetchedAt).toLocaleTimeString()}
            </p>
          </div>

          <div className="screenshot-actions">
            <a className="btn" download={filename} href={imageUrl}>
              Save PNG
            </a>

            <button className="btn modal-close" onClick={onClose} type="button">
              Close
            </button>
          </div>
        </div>

        <div className="screenshot-body">
          <ReactCrop crop={crop} onChange={(c) => setCrop(c)}>
            <img alt="Remote screenshot" className="screenshot-image" src={imageUrl} />
          </ReactCrop>
        </div>
      </div>
    </div>
  );
}

export default ScreenshotModal;
