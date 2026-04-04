import { contextBridge, ipcRenderer } from 'electron';
import { AddPcInput, CommandResult, RemoteCommand, RemotePc } from './types';

contextBridge.exposeInMainWorld('remoteApi', {
  listPcs: (): Promise<RemotePc[]> => ipcRenderer.invoke('agent:list-pcs'),
  addPc: (input: AddPcInput): Promise<RemotePc[]> => ipcRenderer.invoke('agent:add-pc', input),
  removePc: (pcId: string): Promise<RemotePc[]> => ipcRenderer.invoke('agent:remove-pc', { pcId }),
  runCommand: (pcId: string, command: RemoteCommand): Promise<CommandResult> =>
    ipcRenderer.invoke('agent:run-command', { pcId, command }),
  runAgentLine: (pcId: string, line: string): Promise<CommandResult> =>
    ipcRenderer.invoke('agent:run-line', { pcId, line }),
  uploadFile: (pcId: string, localPath: string, remotePath: string): Promise<CommandResult> =>
    ipcRenderer.invoke('agent:upload-file', { pcId, localPath, remotePath }),
  downloadFile: (pcId: string, remotePath: string, localPath: string): Promise<CommandResult> =>
    ipcRenderer.invoke('agent:download-file', { pcId, remotePath, localPath }),
  pickLocalFile: (): Promise<string | null> => ipcRenderer.invoke('agent:pick-local-file'),
  pickSaveFile: (defaultPath?: string): Promise<string | null> =>
    ipcRenderer.invoke('agent:pick-save-file', { defaultPath }),
  listRemoteFiles: (
    pcId: string,
    dir: string,
  ): Promise<{ ok: boolean; message: string; items: string[]; raw: string }> =>
    ipcRenderer.invoke('agent:list-files', { pcId, dir }),
});
