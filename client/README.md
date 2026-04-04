# Client (Electron Forge + React)

Desktop app dùng để điều khiển nhiều máy từ xa (multi-PC dashboard).

**Giao tiếp với Agent (BE):** UI → `preload` → `ipcMain` → TCP tới máy đích. Sơ đồ đầy đủ: [`../docs/architecture.md`](../docs/architecture.md).

## Tech stack

- Electron Forge
- Vite
- React + TypeScript
- Electron IPC (`ipcRenderer` <-> `ipcMain`)

## Prerequisites

- Node.js 18+
- npm 9+

## Install

```bash
cd client
npm install
```

## Run (Development)

```bash
npm start
```

Ghi chú:

- Khi sửa UI React (`src/App.tsx`, components, css), app tự refresh (HMR).
- Khi sửa `src/main.ts` hoặc `src/preload.ts`, gõ `rs` trong terminal `npm start` để restart Electron main process.

## Available scripts

- `npm start` - run app in dev mode
- `npm run lint` - run ESLint
- `npm run package` - package app
- `npm run make` - generate installer/distributables
- `npm run publish` - publish build artifacts

## Current app flow

1. Nhập `IP/hostname` + `port` của agent server.
2. Nhấn `Add Server` để thêm vào danh sách.
3. Chọn server cần điều khiển.
4. Nhấn `Connect Selected` (gửi `PING`) để kiểm tra kết nối.
5. Chạy các lệnh điều khiển khác từ panel chức năng.

## Important files

- `src/main.ts`: Electron main process, TCP connect tới agent, `ipcMain.handle(...)`
- `src/preload.ts`: bridge API an toàn sang renderer (`window.remoteApi`)
- `src/App.tsx`: dashboard UI + form add server + command actions
- `src/components/`: `PcList`, `FeaturePanel`, `SessionConsole`

## IPC channels

- `agent:list-pcs`
- `agent:add-pc`
- `agent:run-command`

## Troubleshooting

- **App không connect được agent**
  - Kiểm tra agent đang chạy đúng port (vd `5050`).
  - Kiểm tra firewall/network giữa 2 máy.
  - Dùng `Connect Selected` để test nhanh bằng `PING`.

- **Sửa code mà không thấy đổi**
  - Đảm bảo chạy bằng `npm start` (dev mode).
  - Với thay đổi `main.ts` / `preload.ts`, gõ `rs` để restart.
