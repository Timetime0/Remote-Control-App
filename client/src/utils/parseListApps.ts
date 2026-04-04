import type { AppListSession, ListAppsAgentResponse, ParsedAppRow, RemotePc } from '../types';

export function parseListAppsRaw(raw: string): ListAppsAgentResponse | null {
  try {
    const parsed = JSON.parse(raw) as {
      ok?: boolean;
      command?: string;
      output?: string;
      items?: unknown;
    };
    if (parsed.ok !== true || parsed.command !== 'LIST_APPS') return null;

    const output = typeof parsed.output === 'string' ? parsed.output : '';
    if (!Array.isArray(parsed.items) && typeof parsed.output !== 'string') return null;

    let items: ListAppsAgentResponse['items'];
    if (Array.isArray(parsed.items)) {
      const list: Array<{ name: string; running: boolean }> = [];
      for (const el of parsed.items) {
        if (
          el &&
          typeof el === 'object' &&
          typeof (el as { name?: unknown }).name === 'string' &&
          typeof (el as { running?: unknown }).running === 'boolean'
        ) {
          list.push({
            name: (el as { name: string }).name,
            running: (el as { running: boolean }).running,
          });
        }
      }
      items = list;
    }

    return {
      ok: true,
      command: 'LIST_APPS',
      output,
      ...(items !== undefined ? { items } : {}),
    };
  } catch {
    // ignore
  }
  return null;
}

export function parseAppOutputToRows(response: ListAppsAgentResponse): ParsedAppRow[] {
  if (response.items !== undefined) {
    return response.items.map((item) => ({
      name: item.name.trim(),
      raw: item.name,
      running: item.running,
    }));
  }
  const lines = response.output.split(/\r?\n/).filter((l) => l.trim().length > 0);
  return lines.map((line) => {
    const name = line.trim();
    return { name, raw: line };
  });
}

export function buildAppListSession(target: RemotePc, raw: string): AppListSession | null {
  const response = parseListAppsRaw(raw);
  if (!response) return null;
  return {
    target,
    response,
    rows: parseAppOutputToRows(response),
    fetchedAt: Date.now(),
  };
}
