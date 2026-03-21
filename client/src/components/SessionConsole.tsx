type SessionConsoleProps = {
  logs: string[];
};

function SessionConsole({ logs }: SessionConsoleProps) {
  return (
    <article className="panel full-width">
      <h2>Session Console</h2>
      <div className="console">
        {logs.length === 0 ? (
          <p>[empty] Chưa có hoạt động.</p>
        ) : (
          logs.map((line) => <p key={line}>{line}</p>)
        )}
      </div>
    </article>
  );
}

export default SessionConsole;
