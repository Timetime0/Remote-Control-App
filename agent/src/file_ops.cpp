#include "file_ops.h"
#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
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

static std::string expandTildeUnixPath(const std::string& path) {
#ifndef _WIN32
    if (path.empty() || path[0] != '~') return path;
    const char* h = std::getenv("HOME");
    if (!h || !*h) return path;
    if (path == "~" || path == "~/") return std::string(h);
    if (path.size() >= 2 && path[1] == '/') return std::string(h) + path.substr(1);
    return path;
#endif
    return path;
}

static std::filesystem::path toFsPath(const std::string& path) {
#ifdef _WIN32
    return std::filesystem::u8path(path);
#else
    return std::filesystem::path(path);
#endif
}

static std::string fsPathToUtf8String(const std::filesystem::path& path) {
#ifdef _WIN32
    const auto u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
#else
    return path.string();
#endif
}

std::string readFileBase64Json(const std::string& path) {
    std::ifstream file(toFsPath(path), std::ios::binary);
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
    std::ofstream file(toFsPath(resolved), std::ios::binary);
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
    const std::string expanded = expandTildeUnixPath(trim(dirPath));
    const std::filesystem::path dir = toFsPath(expanded);
    std::error_code ec;

    if (!std::filesystem::exists(dir, ec) || ec) {
        return "{\"ok\":false,\"command\":\"LIST_FILES\",\"message\":\"dir_not_found\",\"dir\":\"" +
               escapeJson(dirPath) + "\"}";
    }

    if (!std::filesystem::is_directory(dir, ec) || ec) {
        return "{\"ok\":false,\"command\":\"LIST_FILES\",\"message\":\"not_a_directory\",\"dir\":\"" +
               escapeJson(dirPath) + "\"}";
    }

    std::string items = "[";
    bool firstItem = true;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) break;
        const std::string full = fsPathToUtf8String(entry.path());
        if (full.empty()) continue;
        if (!firstItem) items += ",";
        firstItem = false;
        items += "\"" + escapeJson(full) + "\"";
    }
    items += "]";
    return "{\"ok\":true,\"command\":\"LIST_FILES\",\"dir\":\"" + escapeJson(dirPath) +
           "\",\"items\":" + items + "}";
}
