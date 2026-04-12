# Agent (C++)

TCP agent cho máy bị điều khiển, dùng để nhận lệnh từ Electron client. **Chỉ hỗ trợ Windows và macOS** (build trên Linux sẽ báo lỗi biên dịch).

**Giao tiếp với Client:** Agent là **TCP server** (một dòng lệnh `\n` → một dòng JSON). Sơ đồ luồng: [`../docs/architecture.md`](../docs/architecture.md).

## Build

```bash
cmake -S . -B build
cmake --build build
```

Nếu chưa có `cmake`, biên dịch thủ công (macOS, ví dụ):

```bash
c++ -std=c++17 -o remote_agent \
  src/main.cpp src/commands.cpp src/screenshot.cpp src/app_manager.cpp \
  src/ping.cpp src/process_manager.cpp src/system_power.cpp src/file_ops.cpp \
  src/keylogger.cpp -Isrc -framework ApplicationServices
```

(Windows: thêm `-lws2_32` và bỏ `-framework ApplicationServices`.)

## Run

Tham số **cổng** (tùy chọn, mặc định `5050`):

```bash
./build/remote_agent
./build/remote_agent 5050
./build/remote_agent 5051
```

Hoặc sau khi `c++` build ra `remote_agent` trong thư mục hiện tại:

```bash
./remote_agent 5050
```

Mở thêm terminal thứ 2 để chạy agent cho máy giả lập thứ hai:

```bash
./build/remote_agent 5051
```

## Protocol (line-based)

Client gửi command dạng text, mỗi lệnh kết thúc bằng `\n`.

Ví dụ:

- `PING`
- `LIST_PROCESSES`
- `LIST_APPS` — danh sách ứng dụng (macOS: `/Applications`, Windows: Program Files)
- `START_APP_BY_NAME Calculator` — mở app theo tên (macOS: `open -a`, Windows: `Start-Process`)
- `STOP_APP_BY_NAME Safari.app` — thoát app theo tên (macOS: AppleScript `quit`, Windows: `Stop-Process`)
- `STOP_APP_BY_PID 1234` — dừng tiến trình theo PID (giống `KILL_PROCESS`)
- `KILL_PROCESS 1234`
- `SCREENSHOT`, `SHUTDOWN`, `RESTART`, …

Agent trả về một dòng JSON.

## Current status

- `PING`, `LIST_PROCESSES`, `LIST_APPS`, `KILL_PROCESS`, `STOP_APP_BY_PID`, `START_PROCESS`, `START_APP_BY_NAME`: đã có (shell/OS — tùy quyền user)
- Một số lệnh khác: stub `not_implemented_yet`
