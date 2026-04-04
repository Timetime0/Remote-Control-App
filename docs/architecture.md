# Kiến trúc giao tiếp Client ↔ Agent

## Cách “FE nói chuyện” với “BE” trong project này

| Lớp | Vai trò | Công nghệ |
|-----|---------|-----------|
| **UI (FE)** | Giao diện React trong Electron **renderer** | React, `window.remoteApi` |
| **Cầu nối** | Không cho renderer gọi trực tiếp Node/OS; chỉ expose API an toàn | `preload.ts` + `contextBridge` |
| **Main (Electron)** | Chạy TCP client, giữ IPC handlers | `ipcMain.handle`, `node:net` |
| **Agent (BE thật)** | Server TCP trên máy bị điều khiển, thực thi lệnh | C++ (`agent`) |

**Phương thức kết nối giữa máy điều khiển và máy bị điều khiển:** **TCP socket** (plain text: một dòng lệnh `\n`, một dòng JSON trả lời).  
Không phải HTTP REST trong code hiện tại; **Electron IPC** chỉ nội bộ trong app desktop (renderer ↔ main).

---

## Sequence diagram (Mermaid)

```mermaid
sequenceDiagram
    autonumber
    participant U as User (UI)
    participant R as React Renderer
    participant P as Preload (bridge)
    participant M as Electron Main (ipcMain)
    participant T as TCP (node:net)
    participant A as Agent C++ (TCP server)

    U->>R: Click command (vd LIST_PROCESSES)
    R->>P: window.remoteApi.runCommand(pcId, cmd)
    P->>M: ipcRenderer.invoke("agent:run-command", payload)
    M->>M: Lookup host:port theo pcId
    M->>T: socket.connect(host, port)
    T->>A: TCP connect
    M->>T: write(cmd + "\\n")
    T->>A: nhận dòng lệnh
    A->>A: xử lý (shell/OS…)
    A->>T: write(JSON + "\\n")
    T->>M: on("data") đọc 1 dòng
    M-->>P: return { ok, message, raw }
    P-->>R: Promise resolve
    R->>R: Cập nhật UI / popup / log
```

---

## Flowchart — luồng tổng quát (Mermaid)

```mermaid
flowchart TD
    subgraph Electron_Machine["Máy điều khiển (Client)"]
        UI[React UI]
        PRE[Preload API]
        MAIN[Main process<br/>ipcMain + TCP client]
        UI -->|invoke qua contextBridge| PRE
        PRE -->|IPC| MAIN
    end

    subgraph Network["Mạng LAN / IP"]
        NET((TCP IP:port))
    end

    subgraph Target_Machine["Máy bị điều khiển (Agent)"]
        AG[Agent C++<br/>TCP server]
        OS[OS APIs<br/>process, shell, …]
        AG --> OS
    end

    MAIN -->|TCP connect + gửi lệnh| NET
    NET --> AG
    AG -->|JSON 1 dòng| NET
    NET -->|Phản hồi| MAIN
```

---

## Tóm tắt “back and forth”

1. **Trong Electron:** React → `preload` → `ipcMain` (một vòng request/response IPC).
2. **Ra ngoài máy:** `main` mở **TCP** tới `host:port` của Agent, gửi lệnh, nhận JSON.
3. **Agent:** parse lệnh → làm việc với OS → trả JSON.

Nâng cấp sau này có thể thêm **TLS**, **token auth**, hoặc **WebSocket** — kiến trúc IPC + remote TCP vẫn giữ nguyên ý tưởng.
