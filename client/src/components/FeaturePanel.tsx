import { RemoteCommand } from '../types';

type FeaturePanelProps = {
  disabled: boolean;
  onRun: (command: RemoteCommand) => void;
  onClearLogs: () => void;
  onOpenFileTransfer: () => void;
  onOpenMouseControl: () => void;
  onOpenScreenViewer: () => void;
};

const COMMANDS: Array<{ label: string; command: RemoteCommand }> = [
  { label: 'List Applications', command: 'LIST_APPS' },
  { label: 'List Processes', command: 'LIST_PROCESSES' },
  { label: 'Webcam', command: 'WEBCAM_START' },
  { label: 'Screenshot', command: 'SCREENSHOT' },
  { label: 'Keylogger', command: 'KEYLOGGER_START' },
];

function FeaturePanel({
  disabled,
  onRun,
  onClearLogs,
  onOpenFileTransfer,
  onOpenMouseControl,
  onOpenScreenViewer,
}: FeaturePanelProps) {
  return (
    <article className="panel">
      <h2>Remote Control Functions</h2>

      <div className="feature-shortcuts">
        <button
          className="btn primary"
          disabled={disabled}
          onClick={() => onRun('PING')}
          type="button"
        >
          Ping
        </button>

        <button
          className="btn btn-clean"
          onClick={onClearLogs}
          type="button"
        >
          Clean
        </button>

        <button
          className="btn btn-warning"
          disabled={disabled}
          onClick={() => onRun('RESTART')}
          type="button"
        >
          Restart
        </button>

        <button
          className="btn btn-shutdown"
          disabled={disabled}
          onClick={() => onRun('SHUTDOWN')}
          type="button"
        >
          Shutdown
        </button>
      </div>


      <div className="command-grid">
        {COMMANDS.map(({ label, command }) => (
          <button
            className="btn"
            disabled={disabled}
            key={command}
            onClick={() => onRun(command)}
            type="button"
          >
            {label}
          </button>
        ))}

          <button
            className="btn"
            disabled={disabled}
            onClick={onOpenFileTransfer}
            type="button"
          >
            File Transfer
          </button>

          <button
            className="btn"
            disabled={disabled}
            onClick={onOpenMouseControl}
            type="button"
          >
            Mouse Control
          </button>

          <button
            className="btn"
            disabled={disabled}
            onClick={onOpenScreenViewer}
            type="button"
          >
            Screen Viewer
          </button>
      </div>
    </article>
  );
}

export default FeaturePanel;
