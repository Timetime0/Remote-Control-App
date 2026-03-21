# Remote Control App

Ứng dụng điều khiển máy tính từ xa theo mô hình **Client (máy điều khiển)** và **Server/Agent (máy bị điều khiển)**.

## 1) Mục tiêu chức năng

- List / Start / Stop ứng dụng
- List / Start / Stop / Kill process
- Chụp màn hình máy bị điều khiển
- Bắt phím nhấn (keylogger)
- Copy file 2 chiều
- Shutdown / Restart máy bị điều khiển
- Start webcam, record video

## 2) Tech stack đề xuất

### Frontend (Client Desktop UI)

- **Electron**: đóng gói ứng dụng desktop đa nền tảng.
- **React + TypeScript**: xây UI điều khiển và dashboard trạng thái.
- **Vite**: build tool nhanh cho React.
- **TailwindCSS** hoặc **Ant Design**: dựng giao diện nhanh, đồng bộ.
- **Electron IPC**: giao tiếp giữa renderer và main process.

### Backend/Agent (Server - máy bị điều khiển)

- **C++17/20** (khuyến nghị): hiệu năng cao, truy cập API hệ điều hành tốt.
- **Boost.Asio** hoặc **standalone Asio**: networking TCP/WebSocket.
- **nlohmann/json**: encode/decode message JSON.
- **OpenCV**: webcam capture + record video.
- **OS Native APIs**:
  - Windows: Win32 API (process, screenshot, keyboard hook, shutdown).
  - macOS: Quartz/CoreGraphics, IOKit, NSWorkspace, Accessibility API.
  - Linux: X11/Wayland tools, `/proc`, systemd/shutdown commands.

### Kết nối & giao thức

- **WebSocket (TLS)**: realtime command channel.
- **HTTP/HTTPS** (tuỳ chọn): upload/download file, quản lý phiên.
- **Protocol JSON** (giai đoạn đầu), có thể nâng cấp sang **Protobuf** khi ổn định.

### Logging & lưu vết

- **spdlog** (C++): log ở Agent.
- **winston** hoặc **pino** (Node/Electron): log ở Client.
- Lưu file log theo session để phục vụ “All log (Q/A chính)”.

### Build, test, release

- **CMake**: build Agent C++.
- **vcpkg** hoặc **conan**: quản lý dependency C++.
- **pnpm** hoặc **npm**: quản lý package FE.
- **GitHub Actions** (tuỳ chọn): CI build nhiều OS.

## 3) Gợi ý kiến trúc

- **Client App (Electron + React)**
  - UI điều khiển
  - Quản lý danh sách máy đang online
  - Gửi lệnh điều khiển đến Agent
- **Agent Service (C++)**
  - Nhận lệnh từ Client
  - Thực thi theo quyền hệ thống
  - Trả trạng thái, log, file/video/screenshot
- **Signaling/Relay Server** (tuỳ chọn, nếu cần qua Internet)
  - Xác thực, định tuyến kết nối
  - NAT traversal (nâng cao)

## 4) Mapping chức năng -> công nghệ

- **List/Start/Stop app & process**: C++ + OS API (`CreateToolhelp32Snapshot`, `tasklist`, `/proc`...).
- **Kill process**: C++ + OS API (`TerminateProcess`, `kill -9`...).
- **Screenshot**: C++ + native graphics API hoặc thư viện cross-platform.
- **Keylogger**: keyboard hook (Win32 `SetWindowsHookEx`, macOS event tap; cần quyền cao).
- **Copy files**: chunked transfer qua WebSocket/HTTP + checksum SHA-256.
- **Shutdown/Restart**: command hệ thống có kiểm tra quyền.
- **Webcam + Record**: OpenCV + lưu MP4, stream preview nếu cần.

## 5) Yêu cầu đầu ra môn học (Expectation)

- Video demo thuyết trình đầy đủ chức năng.
- Điều khiển **đồng thời 2 PC** trong demo.
- Upload video demo lên YouTube.
- Có **All log** (các câu hỏi/câu trả lời chính, hoặc command/response chính).
- Báo cáo:
  - Thiết kế giao diện phần mềm
  - Mô tả chức năng
  - Giải pháp công nghệ

## 6) Đề xuất phạm vi theo milestone

### Milestone 1 (MVP)

- Kết nối client-agent ổn định
- List process + kill process
- Screenshot
- Shutdown/restart
- Logging cơ bản

### Milestone 2

- Copy file 2 chiều
- Start/stop app nâng cao
- Webcam start/record

### Milestone 3

- Keylogger (nếu được phép theo phạm vi môn học)
- Tối ưu bảo mật, retry, reconnect
- Chuẩn hoá báo cáo + demo 2 PC

## 7) Bảo mật và pháp lý (rất quan trọng)

- Chỉ demo trong môi trường lab/được cho phép.
- Hiển thị cảnh báo xin quyền rõ ràng trên máy bị điều khiển.
- Mã hoá kết nối (TLS), có cơ chế auth/token cho mỗi agent.
- Các tính năng nhạy cảm (keylogger, webcam) cần công tắc bật/tắt và ghi log bắt buộc.

## 8) Cấu trúc repo gợi ý

```txt
remote-control-app/
  client/                 # Electron + React
  agent/                  # C++ Agent (server side)
  shared-protocol/        # JSON schema / protobuf schema
  docs/
    report-outline.md
    architecture.md
```

## 9) Kết luận

Với định hướng **Frontend: React + Electron** và **Backend/Agent: C++**, dự án phù hợp để đạt hiệu năng, kiểm soát sâu hệ điều hành, và đủ sức demo các chức năng điều khiển từ xa trên 2 máy cùng lúc.
