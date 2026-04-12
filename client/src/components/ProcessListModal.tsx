import { useEffect, useState } from 'react';
import AgentListModalShell from './AgentListModalShell';
import type { CommandResult, ParsedProcessRow, ProcessListSession, RemotePc } from '../types';

type ProcessListModalProps = {
  open: boolean;
  session: ProcessListSession | null;
  onClose: () => void;
  onRefresh: (target: RemotePc) => Promise<CommandResult>;
  onRunLine: (target: RemotePc, line: string) => Promise<CommandResult>;
  onLog: (line: string) => void;
};

function ProcessListModal({
  open,
  session,
  onClose,
  onRefresh,
  onRunLine,
  onLog,
}: ProcessListModalProps) {
  const [selectedPid, setSelectedPid] = useState<string | null>(null);
  const [queryText, setQueryText] = useState('');
  const [busy, setBusy] = useState(false);

  useEffect(() => {
    if (!open) {
      setSelectedPid(null);
      setQueryText('');
    }
  }, [open, session?.fetchedAt]);

  if (!open || !session) return null;

  const { target, rows, fetchedAt } = session;

  const normalizedFilter = queryText.trim().toLowerCase();
  const filteredRows =
    normalizedFilter.length === 0
      ? rows
      : rows.filter((r) => {
          const hay = `${r.pid} ${r.comm} ${r.raw}`.toLowerCase();
          return hay.includes(normalizedFilter);
        });

  const handleRefresh = async () => {
    setBusy(true);
    try {
      setSelectedPid(null);
      setQueryText('');
      const result = await onRefresh(target);
      onLog(`${result.ok ? 'OK' : 'FAIL'} LIST_PROCESSES (modal refresh): ${result.message}`);
    } finally {
      setBusy(false);
    }
  };

  const handleKill = async () => {
    if (!selectedPid) {
      onLog('Kill: chọn một dòng process (PID) trước.');
      return;
    }
  
    const selectedRow = rows.find((r) => r.pid === selectedPid);
  
    const confirmed = window.confirm(
      `Do you want to kill "${selectedRow?.comm ?? 'Unknown'}" (PID: ${selectedPid})?\n\nThis action will close the process immediately. Unsaved data may be lost.`
    );
  
    if (!confirmed) {
      onLog(`Cancelled kill PID ${selectedPid}`);
      return;
    }
  
    setBusy(true);
  
    try {
      const line = `KILL_PROCESS ${selectedPid}`;
      const result = await onRunLine(target, line);
      onLog(`${result.ok ? 'OK' : 'FAIL'} ${line} | ${result.raw || result.message}`);
    } finally {
      setBusy(false);
    }
  };

  const handleStart = async () => {
    const trimmed = queryText.trim();
    if (!trimmed) {
      onLog('Start app: nhập tên ứng dụng.');
      return;
    }
    setBusy(true);
    try {
      const line = `START_PROCESS ${trimmed}`;
      const result = await onRunLine(target, line);
      onLog(`${result.ok ? 'OK' : 'FAIL'} ${line} | ${result.raw || result.message}`);
    } finally {
      setBusy(false);
    }
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
        onKeyDown={(e) => e.key === 'Enter' && void handleStart()}
        placeholder="Filter / Start by name (Press ENTER to start)"
        type="text"
        value={queryText}
      />
      <button
        className="btn btn-danger"
        disabled={busy || !selectedPid}
        onClick={() => void handleKill()}
        type="button"
      >
        Kill PID {selectedPid ?? '…'}
      </button>
      <button className="btn" disabled={busy} onClick={() => void handleStart()} type="button">
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
      title="Processes"
      titleId="process-modal-title"
      toolbar={toolbar}
    >
      <table className="process-table">
        <thead>
          <tr>
            <th>PID</th>
            <th>Background process</th>
          </tr>
        </thead>
        <tbody>
          {filteredRows.length === 0 ? (
            <tr>
              <td colSpan={2}>
                <pre className="modal-body-inline">No processes found</pre>
              </td>
            </tr>
          ) : (
            filteredRows.map((row: ParsedProcessRow) => {
              const active = selectedPid === row.pid;
              return (
                <tr
                  className={active ? 'selected' : ''}
                  key={`${row.pid}-${row.raw}`}
                  onClick={() => setSelectedPid(row.pid)}
                  onKeyDown={(e) => e.key === 'Enter' && setSelectedPid(row.pid)}
                  role="button"
                  tabIndex={0}
                >
                  <td className="pid-cell">{row.pid}</td>
                  <td>{row.comm}</td>
                </tr>
              );
            })
          )}
        </tbody>
      </table>
    </AgentListModalShell>
  );
}

export default ProcessListModal;
