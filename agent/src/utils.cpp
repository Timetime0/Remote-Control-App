#include "utils.h"

#include <cstdio>
#include <sstream>

std::string trim(const std::string& input) {
    const auto start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(start, end - start + 1);
}

std::string escapeJson(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
            break;
        }
    }
    return out;
}

std::string jsonCommandResponse(bool ok, const std::string& message, const std::string& command) {
    return "{\"ok\":" + std::string(ok ? "true" : "false") +
           ",\"command\":\"" + command +
           "\",\"message\":\"" + escapeJson(message) + "\"}";
}

std::string runShellCommand(const std::string& command) {
#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (!pipe) {
        return "shell_error";
    }
    char buffer[4096];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return trim(output);
}

std::string joinParts(const std::vector<std::string>& parts, size_t from) {
    std::string out;
    for (size_t i = from; i < parts.size(); ++i) {
        if (i > from) out += ' ';
        out += parts[i];
    }
    return out;
}

std::vector<std::string> splitLines(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream stream(s);
    std::string line;
    while (std::getline(stream, line)) {
        const std::string t = trim(line);
        if (!t.empty()) out.push_back(t);
    }
    return out;
}

std::string base64Encode(const unsigned char* data, size_t len) {
    static const char* table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    size_t i = 0;
    while (i + 2 < len) {
        const unsigned int n = (static_cast<unsigned int>(data[i]) << 16) |
                                 (static_cast<unsigned int>(data[i + 1]) << 8) |
                                 static_cast<unsigned int>(data[i + 2]);
        out.push_back(table[(n >> 18) & 0x3F]);
        out.push_back(table[(n >> 12) & 0x3F]);
        out.push_back(table[(n >> 6) & 0x3F]);
        out.push_back(table[n & 0x3F]);
        i += 3;
    }
    if (i < len) {
        unsigned int n = static_cast<unsigned int>(data[i]) << 16;
        out.push_back(table[(n >> 18) & 0x3F]);
        if (i + 1 < len) {
            n |= static_cast<unsigned int>(data[i + 1]) << 8;
            out.push_back(table[(n >> 12) & 0x3F]);
            out.push_back(table[(n >> 6) & 0x3F]);
            out.push_back('=');
        } else {
            out.push_back(table[(n >> 12) & 0x3F]);
            out.push_back('=');
            out.push_back('=');
        }
    }
    return out;
}

std::string base64Encode(const std::vector<unsigned char>& data) {
    if (data.empty()) return "";
    return base64Encode(data.data(), data.size());
}

bool base64Decode(const std::string& input, std::vector<unsigned char>& out) {
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<int> table(256, -1);
    for (size_t i = 0; i < chars.size(); ++i) {
        table[static_cast<unsigned char>(chars[i])] = static_cast<int>(i);
    }
    out.clear();
    int val = 0;
    int valb = -8;
    for (unsigned char c : input) {
        if (c == '=') break;
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t') continue;
        if (table[c] == -1) return false;
        val = (val << 6) + table[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<unsigned char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return true;
}
