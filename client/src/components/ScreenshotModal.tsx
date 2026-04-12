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

  const [completedCrop, setCompletedCrop] = useState<Crop | null>(null);
  const [imageRef, setImageRef] = useState<HTMLImageElement | null>(null);

  const handleSave = async () => {
    if (!imageRef) return;
  
    const canvas = document.createElement('canvas');
  
    const currentCrop = completedCrop ?? {
      x: 0,
      y: 0,
      width: imageRef.width,
      height: imageRef.height,
    };
  
    const scaleX = imageRef.naturalWidth / imageRef.width;
    const scaleY = imageRef.naturalHeight / imageRef.height;
  
    canvas.width = currentCrop.width ?? 0;
    canvas.height = currentCrop.height ?? 0;
  
    const ctx = canvas.getContext('2d');
    if (!ctx) return;
  
    ctx.drawImage(
      imageRef,
      (currentCrop.x ?? 0) * scaleX,
      (currentCrop.y ?? 0) * scaleY,
      (currentCrop.width ?? 0) * scaleX,
      (currentCrop.height ?? 0) * scaleY,
      0,
      0,
      currentCrop.width ?? 0,
      currentCrop.height ?? 0
    );
  
    const url = canvas.toDataURL('image/png');
  
    const link = document.createElement('a');
    link.download = `screenshot-${Date.now()}.png`;
    link.href = url;
    link.click();

    setCrop({
      unit: '%',
      x: 0,
      y: 0,
      width: 100,
      height: 100,
    });
    
    setCompletedCrop(null);
    onClose();
  };

  if (!open) return null;

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
            <button
              className="btn screenshot-link-btn"
              onClick={() => void handleSave()}
              type="button"
            >
              Save
            </button>

            <button className="btn modal-close" onClick={onClose} type="button">
              Close
            </button>
          </div>
        </div>

        <div className="screenshot-body">
        <ReactCrop
          crop={crop}
          onChange={(c) => setCrop(c)}
          onComplete={(c) => setCompletedCrop(c)}
        >
          <img
            alt="Remote screenshot"
            className="screenshot-image"
            onLoad={(e) => setImageRef(e.currentTarget)}
            src={imageUrl}
          />
        </ReactCrop>
        </div>
      </div>
    </div>
  );
}

export default ScreenshotModal;
