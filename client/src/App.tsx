import { useCallback, useEffect, useState } from 'react';
import FeaturePanel from './components/FeaturePanel';
import PcList from './components/PcList';
import SessionConsole from './components/SessionConsole';
import { AddPcInput, RemoteCommand, RemotePc } from './types';

function App() {
  const [pcs, setPcs] = useState<RemotePc[]>([]);
  const [selectedPc, setSelectedPc] = useState<RemotePc | null>(null);
  const [logs, setLogs] = useState<string[]>([]);
  const [form, setForm] = useState<AddPcInput>({
    name: '',
    host: '127.0.0.1',
    port: 5050,
    os: 'Windows',
  });

  const appendLog = useCallback((line: string) => {
    const ts = new Date().toLocaleTimeString();
    setLogs((prev) => [`[${ts}] ${line}`, ...prev].slice(0, 80));
  }, []);

  const loadPcs = useCallback(async () => {
    try {
      const list = await window.remoteApi.listPcs();
      setPcs(list);
      setSelectedPc((prev) => list.find((item) => item.id === prev?.id) ?? list[0] ?? null);
    } catch (error) {
      appendLog(`Load PCs failed: ${(error as Error).message}`);
    }
  }, [appendLog]);

  useEffect(() => {
    void loadPcs();
    const timer = setInterval(() => {
      void loadPcs();
    }, 5000);
    return () => clearInterval(timer);
  }, [loadPcs]);

  const runCommand = async (command: RemoteCommand) => {
    if (!selectedPc) {
      appendLog('No PC selected.');
      return;
    }
    appendLog(`Run ${command} on ${selectedPc.name}`);
    const result = await window.remoteApi.runCommand(selectedPc.id, command);
    appendLog(
      `${result.ok ? 'OK' : 'FAIL'} ${command}: ${result.message} ${result.raw ? `| ${result.raw}` : ''}`,
    );
    void loadPcs();
  };

  const addServer = async () => {
    try {
      const list = await window.remoteApi.addPc(form);
      setPcs(list);
      const exact = list.find((pc) => pc.host === form.host.trim() && pc.port === Number(form.port));
      if (exact) {
        setSelectedPc(exact);
        appendLog(`Added server 1: ${exact.name} (${exact.host}:${exact.port})`);
      } else {
        appendLog('Server already exists in list.');
      }
    } catch (error) {
      appendLog(`Add server failed: ${(error as Error).message}`);
    }
  };

  return (
    <main className="app">
      <header className="topbar">
        <div>
          <h1>Remote Control Client</h1>
          <p>Điều khiển đồng thời nhiều PC từ một dashboard trung tâm.</p>
        </div>
      </header>

      <section className="panel add-server-panel">
        <h2>Connect to Server Agent</h2>
        <div className="server-form">
          <input
            onChange={(event) =>
              setForm((prev) => ({
                ...prev,
                host: event.target.value,
              }))
            }
            placeholder="IP or hostname"
            type="text"
            value={form.host}
          />
          <input
            min={1}
            onChange={(event) =>
              setForm((prev) => ({
                ...prev,
                port: Number(event.target.value),
              }))
            }
            placeholder="Port"
            type="number"
            value={form.port}
          />
          <input
            onChange={(event) =>
              setForm((prev) => ({
                ...prev,
                name: event.target.value,
              }))
            }
            placeholder="Display name (optional)"
            type="text"
            value={form.name}
          />
          <select
            onChange={(event) =>
              setForm((prev) => ({
                ...prev,
                os: event.target.value as AddPcInput['os'],
              }))
            }
            value={form.os}
          >
            <option value="Windows">Windows</option>
            <option value="macOS">macOS</option>
            <option value="Linux">Linux</option>
          </select>
          <button className="btn primary" onClick={() => void addServer()} type="button">
            Add Server
          </button>
          <button className="btn" onClick={() => void runCommand('PING')} type="button">
            Connect Selected
          </button>
          <button className="btn" onClick={() => void loadPcs()} type="button">
            Refresh
          </button>
        </div>
      </section>

      <section className="selection-row">
        <span>
          Selected:{' '}
          <strong>
            {selectedPc ? `${selectedPc.name} (${selectedPc.host}:${selectedPc.port})` : 'None'}
          </strong>
        </span>
      </section>

      <section className="grid">
        <PcList
          onSelect={(pc) => {
            setSelectedPc(pc);
            appendLog(`Selected ${pc.name}`);
          }}
          pcs={pcs}
          selectedId={selectedPc?.id ?? null}
        />
        <FeaturePanel disabled={!selectedPc} onRun={(command) => void runCommand(command)} />
        <SessionConsole logs={logs} />
      </section>
    </main>
  );
}

export default App;
