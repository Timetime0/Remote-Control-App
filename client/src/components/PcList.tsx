import { RemotePc } from '../types';

type PcListProps = {
  pcs: RemotePc[];
  selectedId: string | null;
  onSelect: (pc: RemotePc) => void;
  onRemove: (pc: RemotePc) => void;
};

function PcList({ pcs, selectedId, onSelect, onRemove }: PcListProps) {
  return (
    <article className="panel">
      <h2>PC List (Multi-PC)</h2>
      <div className="pc-list">
        {pcs.map((pc) => {
          const isSelected = selectedId === pc.id;
          return (
            <div
              className={`pc-card ${isSelected ? 'selected' : ''}`}
              key={pc.id}
              onKeyDown={(e) => e.key === 'Enter' && onSelect(pc)}
              onClick={() => onSelect(pc)}
              role="button"
              tabIndex={0}
            >
              <div>
                <h3>{pc.name}</h3>
                <p>
                  {pc.host}:{pc.port}
                </p>
                <small>{pc.os}</small>
              </div>
              <div className="actions">
                <span className={pc.status === 'Online' ? 'badge online' : 'badge offline'}>
                  {pc.status}
                </span>
                <button
                  aria-label={`Remove ${pc.name}`}
                  className="btn btn-danger btn-icon"
                  onClick={(e) => {
                    e.stopPropagation();
                    onRemove(pc);
                  }}
                  type="button"
                >
                  Remove
                </button>
              </div>
            </div>
          );
        })}
      </div>
    </article>
  );
}

export default PcList;
