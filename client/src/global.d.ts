import { AddPcInput, CommandResult, RemoteCommand, RemotePc } from './types';

declare global {
  interface Window {
    remoteApi: {
      listPcs: () => Promise<RemotePc[]>;
      addPc: (input: AddPcInput) => Promise<RemotePc[]>;
      runCommand: (pcId: string, command: RemoteCommand) => Promise<CommandResult>;
    };
  }
}

export {};
