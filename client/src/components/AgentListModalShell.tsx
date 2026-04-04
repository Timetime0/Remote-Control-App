import type { ReactNode } from 'react';
import type { RemotePc } from '../types';
import { useModalEscape } from '../hooks/useModalEscape';

export type AgentListModalShellProps = {
  open: boolean;
  onClose: () => void;
  title: string;
  titleId: string;
  target: RemotePc;
  fetchedAt: number;
  toolbar: ReactNode;
  children: ReactNode;
};

/**
 * Khung chung cho modal danh sách từ agent (process / apps): backdrop, header, toolbar, vùng scroll.
 */
function AgentListModalShell({
  open,
  onClose,
  title,
  titleId,
  target,
  fetchedAt,
  toolbar,
  children,
}: AgentListModalShellProps) {
  useModalEscape(open, onClose);
  if (!open) return null;

  const timeLabel = new Date(fetchedAt).toLocaleTimeString();

  return (
    <div className="modal-backdrop" onClick={onClose} role="presentation">
      <div className="modal-panel process-modal" onClick={(e) => e.stopPropagation()} role="dialog">
        <div className="modal-header">
          <div>
            <h2 id={titleId}>{title}</h2>
            <p className="modal-sub">
              {target.name} — {target.host}:{target.port} · fetched {timeLabel}
            </p>
          </div>
          <button className="btn modal-close" onClick={onClose} type="button">
            Đóng
          </button>
        </div>
        <div className="process-modal-toolbar">{toolbar}</div>
        <div className="process-table-wrap">{children}</div>
      </div>
    </div>
  );
}

export default AgentListModalShell;
