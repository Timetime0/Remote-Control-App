type SessionConsoleProps = {
  logs: string[];
};

function SessionConsole({ logs }: SessionConsoleProps) {
  return (
    <article className="panel console-panel">
      <h2>Session Console</h2>

      <div className="console-output">
        {logs.length === 0 ? (
          <pre>[empty] No activity yet.</pre>
        ) : (
          logs.map((line, index) => <pre key={`${index}-${line}`}>{line}</pre>)
        )}
      </div>
    </article>
  );
}

export default SessionConsole;
