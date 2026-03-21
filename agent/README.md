# Agent (C++)

TCP agent cho máy bị điều khiển, dùng để nhận lệnh từ Electron client.

## Build

```bash
cmake -S . -B build
cmake --build build
```

Nếu chưa có `cmake`, có thể compile nhanh:

```bash
c++ -std=c++17 src/main.cpp -o remote_agent
```

## Run

```bash
./build/remote_agent 5050
```

Hoặc nếu dùng lệnh compile nhanh:

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
- `SCREENSHOT`
- `SHUTDOWN`
- `RESTART`

Agent trả về một dòng JSON.

## Current status

- `PING`: đã hoạt động
- `LIST_PROCESSES`: đã hoạt động (shell based)
- Các lệnh còn lại: đã có stub response `not_implemented_yet` để tích hợp UI/demo flow trước
