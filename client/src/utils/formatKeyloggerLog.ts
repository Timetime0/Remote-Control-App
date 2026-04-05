/**
 * macOS virtual key codes (CGEvent / Carbon), US ANSI layout for letter/number rows.
 * @see HIToolbox Events.h
 */
const MAC_VIRTUAL_KEY_NAMES: Record<number, string> = {
  // Letters & numbers (layout-dependent row)
  0: 'A',
  1: 'S',
  2: 'D',
  3: 'F',
  4: 'H',
  5: 'G',
  6: 'Z',
  7: 'X',
  8: 'C',
  9: 'V',
  11: 'B',
  12: 'Q',
  13: 'W',
  14: 'E',
  15: 'R',
  16: 'Y',
  17: 'T',
  18: '1',
  19: '2',
  20: '3',
  21: '4',
  22: '6',
  23: '5',
  24: '=',
  25: '9',
  26: '7',
  27: '-',
  28: '8',
  29: '0',
  30: ']',
  31: 'O',
  32: 'U',
  33: '[',
  34: 'I',
  35: 'P',
  36: 'Return',
  37: 'L',
  38: 'J',
  39: "'",
  40: 'K',
  41: ';',
  42: '\\',
  43: ',',
  44: '/',
  45: 'N',
  46: 'M',
  47: '.',
  48: 'Tab',
  49: 'Space',
  50: '`',
  51: 'Delete',
  53: 'Escape',
  // Modifiers & special
  55: 'Command',
  56: 'Shift',
  57: 'CapsLock',
  58: 'Option',
  59: 'Control',
  60: 'Right Shift',
  61: 'Right Option',
  62: 'Right Control',
  63: 'Fn',
  64: 'F17',
  72: 'VolumeUp',
  73: 'VolumeDown',
  74: 'Mute',
  79: 'F18',
  80: 'F19',
  90: 'F20',
  96: 'F5',
  97: 'F6',
  98: 'F7',
  99: 'F3',
  100: 'F8',
  101: 'F9',
  103: 'F11',
  105: 'F13',
  106: 'F16',
  107: 'F14',
  109: 'F10',
  111: 'F12',
  113: 'F15',
  114: 'Help',
  115: 'Home',
  116: 'PageUp',
  117: 'ForwardDelete',
  118: 'F4',
  119: 'End',
  120: 'F2',
  121: 'PageDown',
  122: 'F1',
  123: 'Left',
  124: 'Right',
  125: 'Down',
  126: 'Up',
};

function macVirtualKeyLabel(code: number): string {
  return MAC_VIRTUAL_KEY_NAMES[code] ?? `keycode ${code}`;
}

const KEYCODE_LINE = /^(\d+)\s+keycode=(\d+)\s*$/;

function formatOneLine(line: string): string {
  const trimmed = line.trimEnd();
  const m = trimmed.match(KEYCODE_LINE);
  if (!m) return line;

  const sec = parseInt(m[1], 10);
  const keyCode = parseInt(m[2], 10);
  const d = new Date(sec * 1000);
  const timeStr = Number.isFinite(d.getTime())
    ? d.toLocaleString(undefined, { dateStyle: 'short', timeStyle: 'medium' })
    : m[1];
  const key = macVirtualKeyLabel(keyCode);
  return `${timeStr}  ${key}`;
}

/** Raw agent log → local time + key labels (macOS US layout). */
export function formatKeyloggerForDisplay(raw: string): string {
  if (!raw.trim()) return raw;
  return raw.split(/\r?\n/).map((ln) => formatOneLine(ln)).join('\n');
}
