import { contextBridge, ipcRenderer } from 'electron';
import { AddPcInput, CommandResult, RemoteCommand, RemotePc } from './types';

contextBridge.exposeInMainWorld('remoteApi', {
  listPcs: (): Promise<RemotePc[]> => ipcRenderer.invoke('agent:list-pcs'),
  addPc: (input: AddPcInput): Promise<RemotePc[]> => ipcRenderer.invoke('agent:add-pc', input),
  runCommand: (pcId: string, command: RemoteCommand): Promise<CommandResult> =>
    ipcRenderer.invoke('agent:run-command', { pcId, command }),
});
