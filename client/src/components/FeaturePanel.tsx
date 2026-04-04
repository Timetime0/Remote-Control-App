import { RemoteCommand } from '../types';

type FeaturePanelProps = {
  disabled: boolean;
  onRun: (command: RemoteCommand) => void;
  onOpenKeylogger: () => void;
  onOpenWebcam: () => void;
  onOpenFileTransfer: () => void;
};

const COMMANDS: Array<{ label: string; command: RemoteCommand }> = [
  { label: 'Ping Agent', command: 'PING' },
  { label: 'List Processes', command: 'LIST_PROCESSES' },
  { label: 'List Applications', command: 'LIST_APPS' },
  { label: 'Screenshot', command: 'SCREENSHOT' },
  { label: 'Shutdown', command: 'SHUTDOWN' },
  { label: 'Restart', command: 'RESTART' },
];

function FeaturePanel({
  disabled,
  onRun,
  onOpenKeylogger,
  onOpenWebcam,
  onOpenFileTransfer,
}: FeaturePanelProps) {
  return (
    <article className="panel">
      <h2>Chức năng điều khiển từ xa</h2>
      <div className="feature-shortcuts">
        <button className="btn primary" disabled={disabled} onClick={onOpenKeylogger} type="button">
          Keylogger
        </button>
        <button className="btn primary" disabled={disabled} onClick={onOpenWebcam} type="button">
          Webcam
        </button>
        <button
          className="btn primary"
          disabled={disabled}
          onClick={onOpenFileTransfer}
          type="button"
        >
          File
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
      </div>
    </article>
  );
}

export default FeaturePanel;
