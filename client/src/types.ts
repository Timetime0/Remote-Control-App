export type RemotePc = {
  id: string;
  name: string;
  host: string;
  port: number;
  status: 'Online' | 'Offline';
  os: 'Windows' | 'macOS' ;
};

export type RemoteCommand =
  | 'PING'
  | 'LIST_PROCESSES'
  | 'LIST_APPS'
  | 'SCREENSHOT'
  | 'KEYLOGGER_START'
  | 'KEYLOGGER_STOP'
  | 'KEYLOGGER_GET_LOG'
  | 'SHUTDOWN'
  | 'RESTART'
  | 'WEBCAM_START'
  | 'WEBCAM_STOP'
  | 'WEBCAM_RECORD_START'
  | 'WEBCAM_RECORD_STOP'
  | 'MOUSE_MOVE'
  | 'MOUSE_LEFT_CLICK'
  | 'MOUSE_RIGHT_CLICK'
  | 'SCREEN_VIEWER_START'
  | 'SCREEN_VIEWER_STOP';

export type CommandResult = {
  ok: boolean;
  message: string;
  raw: string;
};

export type AddPcInput = {
  name: string;
  host: string;
  port: number;
  os: 'Windows' | 'macOS' ;
};

/** JSON một dòng từ agent khi command = LIST_PROCESSES */
export type ListProcessesAgentResponse = {
  ok: true;
  command: 'LIST_PROCESSES';
  output: string;
};

/** Một dòng process đã parse (ps: pid + comm, hoặc raw) */
export type ParsedProcessRow = {
  pid: string;
  comm: string;
  raw: string;
};

/**
 * Snapshot khi đã list process: gắn với máy đích (giống RemotePc) + dữ liệu + thời điểm fetch.
 * Dùng để mở modal và bấm Refresh / Kill / Start.
 */
export type ProcessListSession = {
  target: RemotePc;
  response: ListProcessesAgentResponse;
  rows: ParsedProcessRow[];
  fetchedAt: number;
};

/** JSON một dòng từ agent khi command = LIST_APPS */
export type ListAppsAgentResponse = {
  ok: true;
  command: 'LIST_APPS';
  output: string;
  /** Khi agent hỗ trợ: tên + trạng thái đang chạy */
  items?: Array<{ name: string; running: boolean }>;
};

export type ParsedAppRow = {
  name: string;
  raw: string;
  /** Chỉ có khi agent trả `items`; không có khi fallback parse từ `output` */
  running?: boolean;
};

export type AppListSession = {
  target: RemotePc;
  response: ListAppsAgentResponse;
  rows: ParsedAppRow[];
  fetchedAt: number;
};
