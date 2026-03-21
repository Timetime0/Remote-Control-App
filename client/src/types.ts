export type RemotePc = {
  id: string;
  name: string;
  host: string;
  port: number;
  status: 'Online' | 'Offline';
  os: 'Windows' | 'macOS' | 'Linux';
};

export type RemoteCommand =
  | 'PING'
  | 'LIST_PROCESSES'
  | 'SCREENSHOT'
  | 'SHUTDOWN'
  | 'RESTART'
  | 'WEBCAM_START'
  | 'WEBCAM_RECORD_START'
  | 'WEBCAM_RECORD_STOP';

export type CommandResult = {
  ok: boolean;
  message: string;
  raw: string;
};

export type AddPcInput = {
  name: string;
  host: string;
  port: number;
  os: 'Windows' | 'macOS' | 'Linux';
};
