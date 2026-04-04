import type {
  ListProcessesAgentResponse,
  ParsedProcessRow,
  ProcessListSession,
  RemotePc,
} from '../types';

/**
 * Parse JSON một dòng từ agent cho LIST_PROCESSES.
 */
export function parseListProcessesRaw(raw: string): ListProcessesAgentResponse | null {
  try {
    const parsed = JSON.parse(raw) as {
      ok?: boolean;
      command?: string;
      output?: string;
    };
    if (
      parsed.ok === true &&
      parsed.command === 'LIST_PROCESSES' &&
      typeof parsed.output === 'string'
    ) {
      return {
        ok: true,
        command: 'LIST_PROCESSES',
        output: parsed.output,
      };
    }
  } catch {
    // ignore
  }
  return null;
}

/**
 * Parse output kiểu `ps -axo pid,comm` hoặc tasklist (fallback: một cột raw).
 */
export function parseProcessOutputToRows(output: string): ParsedProcessRow[] {
  const lines = output.split(/\r?\n/).filter((l) => l.trim().length > 0);
  const rows: ParsedProcessRow[] = [];

  for (const line of lines) {
    const trimmed = line.trim();
    if (/^PID\b/i.test(trimmed)) continue;

    const m = trimmed.match(/^(\d+)\s+(.+)$/);
    if (m) {
      rows.push({
        pid: m[1],
        comm: m[2].trim(),
        raw: line,
      });
      continue;
    }

    const win = trimmed.match(/^(.+?)\s+(\d+)\s+/);
    if (win) {
      rows.push({
        pid: win[2],
        comm: win[1].trim(),
        raw: line,
      });
    }
  }

  return rows;
}

export function buildProcessListSession(target: RemotePc, raw: string): ProcessListSession | null {
  const response = parseListProcessesRaw(raw);
  if (!response) return null;
  return {
    target,
    response,
    rows: parseProcessOutputToRows(response.output),
    fetchedAt: Date.now(),
  };
}
