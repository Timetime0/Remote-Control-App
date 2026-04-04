import { AddPcInput, CommandResult, RemoteCommand, RemotePc } from './types';

declare global {
  interface Window {
    remoteApi: {
      listPcs: () => Promise<RemotePc[]>;
      addPc: (input: AddPcInput) => Promise<RemotePc[]>;
      removePc: (pcId: string) => Promise<RemotePc[]>;
      runCommand: (pcId: string, command: RemoteCommand) => Promise<CommandResult>;
      runAgentLine: (pcId: string, line: string) => Promise<CommandResult>;
      uploadFile: (pcId: string, localPath: string, remotePath: string) => Promise<CommandResult>;
      downloadFile: (pcId: string, remotePath: string, localPath: string) => Promise<CommandResult>;
      pickLocalFile: () => Promise<string | null>;
      pickSaveFile: (defaultPath?: string) => Promise<string | null>;
      listRemoteFiles: (
        pcId: string,
        dir: string,
      ) => Promise<{ ok: boolean; message: string; items: string[]; raw: string }>;
    };
  }
}

export {};
