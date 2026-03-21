import { RemoteCommand } from '../types';

type FeaturePanelProps = {
  disabled: boolean;
  onRun: (command: RemoteCommand) => void;
};

const COMMANDS: Array<{ label: string; command: RemoteCommand }> = [
  { label: 'Ping Agent', command: 'PING' },
  { label: 'List Processes', command: 'LIST_PROCESSES' },
  { label: 'Screenshot', command: 'SCREENSHOT' },
  { label: 'Shutdown', command: 'SHUTDOWN' },
  { label: 'Restart', command: 'RESTART' },
  { label: 'Start Webcam', command: 'WEBCAM_START' },
  { label: 'Start Record Webcam', command: 'WEBCAM_RECORD_START' },
  { label: 'Stop Record Webcam', command: 'WEBCAM_RECORD_STOP' },
];

function FeaturePanel({ disabled, onRun }: FeaturePanelProps) {
  return (
    <article className="panel">
      <h2>Chức năng điều khiển từ xa</h2>
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
