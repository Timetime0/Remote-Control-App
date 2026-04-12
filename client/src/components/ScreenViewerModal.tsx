import { useModalEscape } from '../hooks/useModalEscape';

type ScreenViewerModalProps = {
  open: boolean;
  imageUrl: string;
  targetLabel: string;
  onClose: () => void;
};

function ScreenViewerModal({
  open,
  imageUrl,
  targetLabel,
  onClose,
}: ScreenViewerModalProps) {
  useModalEscape(open, onClose);

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
            <h2>Screen Viewer</h2>
            <p className="modal-sub">{targetLabel}</p>
          </div>

          <button className="btn modal-close" onClick={onClose} type="button">
            Close
          </button>
        </div>

        <div className="screenshot-body">
          {imageUrl ? (
            <img alt="Live screen" className="screenshot-image" src={imageUrl} />
          ) : (
            <div className="empty-state">Waiting for screen...</div>
          )}
        </div>
      </div>
    </div>
  );
}

export default ScreenViewerModal;