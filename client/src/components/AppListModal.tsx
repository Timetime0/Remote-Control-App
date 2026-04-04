import { useEffect, useState } from 'react';
import AgentListModalShell from './AgentListModalShell';
import type { AppListSession, CommandResult, ParsedAppRow, RemotePc } from '../types';

type AppListModalProps = {
  open: boolean;
  session: AppListSession | null;
  onClose: () => void;
  onRefresh: (target: RemotePc) => Promise<CommandResult>;
  onRunLine: (target: RemotePc, line: string) => Promise<CommandResult>;
  onLog: (line: string) => void;
};

function AppListModal({ open, session, onClose, onRefresh, onRunLine, onLog }: AppListModalProps) {
  const [queryText, setQueryText] = useState('');
  const [busy, setBusy] = useState(false);

  useEffect(() => {
    if (!open) {
      setQueryText('');
    }
  }, [open, session?.fetchedAt]);

  if (!open || !session) return null;

  const { target, rows, fetchedAt } = session;

  const normalizedFilter = queryText.trim().toLowerCase();
  const filteredRows =
    normalizedFilter.length === 0
      ? rows
      : rows.filter((r) => r.name.toLowerCase().includes(normalizedFilter));

  const handleRefresh = async () => {
    setBusy(true);
    try {
      setQueryText('');
      const result = await onRefresh(target);
      onLog(`${result.ok ? 'OK' : 'FAIL'} LIST_APPS (modal refresh): ${result.message}`);
    } finally {
      setBusy(false);
    }
  };

  const runStartByName = async (name: string) => {
    const trimmed = name.trim();
    if (!trimmed) {
      onLog('Start app: nhập hoặc chọn tên ứng dụng.');
      return;
    }
    setBusy(true);
    try {
      const line = `START_APP_BY_NAME ${trimmed}`;
      const result = await onRunLine(target, line);
      onLog(`${result.ok ? 'OK' : 'FAIL'} ${line} | ${result.raw || result.message}`);
    } finally {
      setBusy(false);
    }
  };

  const runStopByName = async (name: string) => {
    const trimmed = name.trim();
    if (!trimmed) {
      onLog('Stop: nhập hoặc chọn tên ứng dụng.');
      return;
    }
    setBusy(true);
    try {
      const line = `STOP_APP_BY_NAME ${trimmed}`;
      const result = await onRunLine(target, line);
      onLog(`${result.ok ? 'OK' : 'FAIL'} ${line} | ${result.raw || result.message}`);
    } finally {
      setBusy(false);
    }
  };

  const handleStartByQuery = async () => {
    await runStartByName(queryText);
  };

  const toolbar = (
    <>
      <button
        className="btn primary"
        disabled={busy}
        onClick={() => void handleRefresh()}
        type="button"
      >
        Refresh
      </button>
      <input
        className="process-start-input"
        onChange={(e) => setQueryText(e.target.value)}
        onKeyDown={(e) => e.key === 'Enter' && void handleStartByQuery()}
        placeholder="Filter / Start by name (Enter để start)"
        type="text"
        value={queryText}
      />
      <button
        className="btn"
        disabled={busy}
        onClick={() => void handleStartByQuery()}
        type="button"
      >
        Start by name
      </button>
    </>
  );

  return (
    <AgentListModalShell
      fetchedAt={fetchedAt}
      onClose={onClose}
      open={open}
      target={target}
      title="Applications"
      titleId="app-modal-title"
      toolbar={toolbar}
    >
      <table className="process-table app-table">
        <thead>
          <tr>
            <th>Application</th>
            <th>Status</th>
            <th>Action</th>
          </tr>
        </thead>
        <tbody>
          {filteredRows.length === 0 ? (
            <tr>
              <td colSpan={3}>
                <pre className="modal-body-inline">No applications found</pre>
              </td>
            </tr>
          ) : (
            filteredRows.map((row: ParsedAppRow, index: number) => {
              return (
                <tr key={`${index}-${row.raw}`}>
                  <td>{row.name}</td>
                  <td>
                    {row.running === undefined ? (
                      <span className="muted">—</span>
                    ) : row.running ? (
                      <span className="badge online">Running</span>
                    ) : (
                      <span className="badge offline">Stopped</span>
                    )}
                  </td>
                  <td>
                    {row.running === false ? (
                      <button
                        className="btn btn-primary"
                        disabled={busy}
                        onClick={() => void runStartByName(row.name)}
                        type="button"
                      >
                        Start
                      </button>
                    ) : row.running === true ? (
                      <button
                        className="btn btn-danger"
                        disabled={busy}
                        onClick={() => void runStopByName(row.name)}
                        type="button"
                      >
                        Stop
                      </button>
                    ) : (
                      <span className="muted">—</span>
                    )}
                  </td>
                </tr>
              );
            })
          )}
        </tbody>
      </table>
    </AgentListModalShell>
  );
}

export default AppListModal;
