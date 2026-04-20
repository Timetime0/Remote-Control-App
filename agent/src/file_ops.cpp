#include "file_ops.h"
#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

static bool isBareFileNameForWrite(const std::string& path) {
    if (path.empty()) return false;
    if (path == "." || path == "..") return false;
#ifdef _WIN32
    if (path.find_first_of("/\\:") != std::string::npos) return false;
#else
    if (path.find('/') != std::string::npos) return false;
#endif
    return true;
}

static std::string desktopDirectoryPath() {
#ifdef _WIN32
    const char* profile = std::getenv("USERPROFILE");
    if (!profile || !*profile) return "";
    std::string d(profile);
    while (!d.empty() && (d.back() == '\\' || d.back() == '/')) d.pop_back();
    d += "\\Desktop";
    return d;
#else
    const char* home = std::getenv("HOME");
    if (!home || !*home) return "";
    std::string d(home);
    while (!d.empty() && d.back() == '/') d.pop_back();
    d += "/Desktop";
    return d;
#endif
}

static std::string resolveWritePath(const std::string& path) {
    if (!isBareFileNameForWrite(path)) return path;
    const std::string desk = desktopDirectoryPath();
    if (desk.empty()) return path;
#ifdef _WIN32
    return desk + "\\" + path;
#else
    return desk + "/" + path;
#endif
}

#ifndef _WIN32
static std::string expandTildeUnixPath(const std::string& path) {
    if (path.empty() || path[0] != '~') return path;
    const char* h = std::getenv("HOME");
    if (!h || !*h) return path;
    if (path == "~" || path == "~/") return std::string(h);
    if (path.size() >= 2 && path[1] == '/') return std::string(h) + path.substr(1);
    return path;
}
#endif

static std::string shellSingleQuote(const std::string& s) {
    std::string r = "'";
    for (char c : s) {
        if (c == '\'') r += "'\\''";
        else r += c;
    }
    r += "'";
    return r;
}

#ifdef _WIN32
static std::string escapeSingleQuotesPowerShell(const std::string& input) {
    std::string out;
    for (char c : input) {
        if (c == '\'') out += "''";
        else out += c;
    }
    return out;
}
#endif

std::string readFileBase64Json(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "{\"ok\":false,\"command\":\"READ_FILE_BASE64\",\"message\":\"open_failed\"}";
    }
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size < 0) {
        return "{\"ok\":false,\"command\":\"READ_FILE_BASE64\",\"message\":\"read_failed\"}";
    }
    std::vector<unsigned char> buffer(static_cast<size_t>(size));
    if (size > 0 && !file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return "{\"ok\":false,\"command\":\"READ_FILE_BASE64\",\"message\":\"read_failed\"}";
    }
    const std::string b64 = base64Encode(buffer);
    return "{\"ok\":true,\"command\":\"READ_FILE_BASE64\",\"path\":\"" + escapeJson(path) +
           "\",\"data\":\"" + b64 + "\"}";
}

std::string writeFileBase64Json(const std::string& path, const std::string& dataB64) {
    std::vector<unsigned char> bytes;
    if (!base64Decode(dataB64, bytes)) {
        return "{\"ok\":false,\"command\":\"WRITE_FILE_BASE64\",\"message\":\"invalid_base64\"}";
    }
    const std::string resolved = resolveWritePath(path);
    std::ofstream file(resolved, std::ios::binary);
    if (!file) {
        return "{\"ok\":false,\"command\":\"WRITE_FILE_BASE64\",\"message\":\"open_failed\"}";
    }
    if (!bytes.empty()) {
        file.write(reinterpret_cast<const char*>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()));
    }
    if (!file) {
        return "{\"ok\":false,\"command\":\"WRITE_FILE_BASE64\",\"message\":\"write_failed\"}";
    }
    const std::string requestedEsc = escapeJson(path);
    const std::string resolvedEsc = escapeJson(resolved);
    return "{\"ok\":true,\"command\":\"WRITE_FILE_BASE64\",\"requested\":\"" + requestedEsc +
           "\",\"path\":\"" + resolvedEsc + "\",\"bytes\":" + std::to_string(bytes.size()) + "}";
}

std::string listFilesJson(const std::string& dirPath) {
#ifdef _WIN32
    const std::string cmd =
        "powershell -NoProfile -Command \"Get-ChildItem -LiteralPath '" +
        escapeSingleQuotesPowerShell(dirPath) +
        "' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName\"";
    const std::string out = runShellCommand(cmd);
    std::vector<std::string> lines = splitLines(out);
    std::string items = "[";
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) items += ",";
        items += "\"" + escapeJson(lines[i]) + "\"";
    }
    items += "]";
#else
    const std::string expanded = expandTildeUnixPath(trim(dirPath));
    const std::string cmd =
        "sh -c \"ls -1p " + shellSingleQuote(expanded) + " 2>/dev/null | tr -d '\\r'\"";
    const std::string out = runShellCommand(cmd);
    std::vector<std::string> lines = splitLines(out);
    std::string items = "[";
    bool firstItem = true;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string name = lines[i];
        while (!name.empty() && (name.back() == '/' || name.back() == '\\')) name.pop_back();
        if (name.empty()) continue;
        if (!firstItem) items += ",";
        firstItem = false;
        std::string full = expanded;
        if (!full.empty() && full.back() != '/' && full.back() != '\\') full += "/";
        full += name;
        items += "\"" + escapeJson(full) + "\"";
    }
    items += "]";
#endif
    return "{\"ok\":true,\"command\":\"LIST_FILES\",\"dir\":\"" + escapeJson(dirPath) +
           "\",\"items\":" + items + "}";
}
