#pragma once

#include <string>
#include <vector>

/** Chuẩn hóa chuỗi đầu/cuối (khoảng trắng, tab, CR/LF). */
std::string trim(const std::string& input);

/**
 * Escape chuỗi đặt trong JSON (dấu ngoặc kép): \\ " \\n \\r \\t
 * Dùng cho message/path/output gửi trong JSON.
 */
std::string escapeJson(const std::string& input);

/** {"ok":true|false,"command":"...","message":"..."} — message đã escape. */
std::string jsonCommandResponse(bool ok, const std::string& message,
                                const std::string& command = "");

/** Chạy lệnh shell, đọc stdout; Windows dùng _popen, Unix dùng popen. Trả rỗng nếu lỗi pipe. */
std::string runShellCommand(const std::string& command);

/** Nối các token vector từ chỉ số `from` bằng khoảng trắng (đường dẫn có space). */
std::string joinParts(const std::vector<std::string>& parts, size_t from);

/** Tách dòng, trim từng dòng, bỏ dòng rỗng. */
std::vector<std::string> splitLines(const std::string& s);

std::string base64Encode(const std::vector<unsigned char>& data);
std::string base64Encode(const unsigned char* data, size_t len);

bool base64Decode(const std::string& input, std::vector<unsigned char>& out);
