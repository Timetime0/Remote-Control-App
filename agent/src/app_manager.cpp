#include "app_manager.h"

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <set>

#ifdef _WIN32
#include <windows.h>
#include <winver.h>
#include <processthreadsapi.h>
#include <tlhelp32.h>
#include <shellapi.h>
#pragma comment(lib, "Version.lib")
#endif

#include <filesystem>
namespace fs = std::filesystem;

// ===== UTILS =====
static std::string escapeJson(const std::string& input) {
    std::string out;
    for (char c : input) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

static std::string json(bool ok, const std::string& msg, const std::string& cmd) {
    return "{\"ok\":" + std::string(ok ? "true" : "false") +
        ",\"command\":\"" + cmd +
        "\",\"message\":\"" + escapeJson(msg) + "\"}";
}

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static std::string resolveAlias(const std::string& input) {
    std::string s = toLower(input);

    if (s == "paint") return "mspaint.exe";
    if (s == "note") return "notepad.exe";
    if (s == "calc") return "calc.exe";
    if (s == "cmd") return "cmd.exe";
    if (s == "word") return "winword.exe";
    if (s == "excel") return "excel.exe";
    if (s == "powerpoint") return "powerpnt.exe";

    return "";
}

static bool match(const std::string& name, const std::string& input) {
    std::string a = toLower(name);
    std::string b = toLower(input);

    // chua
    if (a.find(b) != std::string::npos) return true;

    // viet tat pt -> paint
    int j = 0;
    for (char c : a) {
        if (j < b.size() && c == b[j]) j++;
    }
    return j == b.size();
}

static std::string getFileName(const std::string& path) {
    size_t pos = path.find_last_of("\\/");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

static std::string getDisplayName(const std::string& path) {
    DWORD dummy;
    DWORD size = GetFileVersionInfoSizeA(path.c_str(), &dummy);

    if (size == 0) return getFileName(path);

    std::vector<char> data(size);

    if (!GetFileVersionInfoA(path.c_str(), 0, size, data.data()))
        return getFileName(path);

    void* value = nullptr;
    UINT len = 0;

    if (VerQueryValueA(
        data.data(),
        "\\StringFileInfo\\040904b0\\FileDescription",
        &value,
        &len)) {

        return std::string((char*)value);
    }

    return getFileName(path);
}


#ifdef _WIN32

struct AppEntry {
    std::string name;
    std::string path;
};

static std::vector<AppEntry> g_cache;

static void buildCache() {
    if (!g_cache.empty()) return;

    std::vector<std::string> paths = {
    "C:\\Windows\\System32",
    "C:\\Program Files",
    "C:\\Program Files (x86)",
    "C:\\ProgramData\\Microsoft\\Windows\\Start Menu"
    };

    char* appdata = getenv("APPDATA");
    if (appdata) {
        paths.push_back(std::string(appdata) + "\\Microsoft\\Windows\\Start Menu");
    }

    for (const auto& base : paths) {
        try {
            for (auto& p : fs::recursive_directory_iterator(base)) {
                auto ext = p.path().extension().string();

                if (ext == ".exe" || ext == ".lnk") {
                    std::string full = p.path().string();
                    std::string name = getFileName(full);
                    g_cache.push_back({ name, full });
                }
            }
        }
        catch (...) {}
    }
}

// ===== STRUCT =====
struct AppInfo {
    int pid;
    std::string name;
    std::string path;
};

// ===== ENUM WINDOW =====
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    if (!IsWindowVisible(hwnd)) return TRUE;

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) return TRUE;

    char path[MAX_PATH] = "";
    DWORD size = MAX_PATH;

    if (QueryFullProcessImageNameA(hProcess, 0, path, &size)) {
        std::string exePath = path;
        std::string name = getFileName(exePath);

        auto* list = (std::vector<AppInfo>*)lParam;
        list->push_back({ (int)pid, name, exePath });
    }

    CloseHandle(hProcess);
    return TRUE;
}

// ===== LIST APPS =====
std::string cmdListApps() {
    std::vector<AppInfo> apps;
    EnumWindows(EnumWindowsProc, (LPARAM)&apps);

    std::set<std::string> seen;
    std::ostringstream oss;
    oss << "[";

    bool first = true;

    for (auto& app : apps) {
        std::string key = toLower(app.name);

        // Bo trung lap
        if (seen.count(key)) continue;
        seen.insert(key);

        if (!first) oss << ",";
        first = false;

        std::string display = getDisplayName(app.path);

        oss << "{"
            << "\"name\":\"" << escapeJson(display) << "\","
            << "\"running\":true,"
            << "\"raw\":\"" << escapeJson(app.name) << "\","
            << "\"path\":\"" << escapeJson(app.path) << "\""
            << "}";
    }

    oss << "]";

    return "{\"ok\":true,\"command\":\"LIST_APPS\",\"items\":" + oss.str() + "}";
}

// ===== START APP =====
std::string cmdStart(const std::vector<std::string>& parts) {
    if (parts.size() < 2)
        return json(false, "missing_app", "START_APP_BY_NAME");

    std::string target = parts[1];

    // ===== 1. ALIAS =====
    std::string alias = resolveAlias(target);
    if (!alias.empty()) {
        HINSTANCE res = ShellExecuteA(NULL, "open", alias.c_str(), NULL, NULL, SW_SHOWNORMAL);
        if ((INT_PTR)res > 32)
            return json(true, "started " + alias, "START_APP_BY_NAME");
    }

    // ===== 2. TRY DIRECT =====
    std::string tryExe = target;
    if (tryExe.find(".exe") == std::string::npos)
        tryExe += ".exe";

    HINSTANCE res = ShellExecuteA(NULL, "open", tryExe.c_str(), NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)res > 32)
        return json(true, "started " + tryExe, "START_APP_BY_NAME");

    // ===== 3. SEARCH CACHE =====
    buildCache();

    for (const auto& app : g_cache) {
        if (match(app.name, target)) {
            HINSTANCE r = ShellExecuteA(NULL, "open", app.path.c_str(), NULL, NULL, SW_SHOWNORMAL);

            if ((INT_PTR)r > 32)
                return json(true, "started " + app.name, "START_APP_BY_NAME");
        }
    }

    return json(false, "not_found", "START_APP_BY_NAME");
}

// ===== STOP APP =====
std::string cmdStop(const std::vector<std::string>& parts) {
    if (parts.size() < 2)
        return json(false, "missing_name", "STOP_APP_BY_NAME");

    std::string target = toLower(parts[1]);

    if (target.find(".exe") == std::string::npos)
        target += ".exe";

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return json(false, "snapshot_fail", "STOP_APP_BY_NAME");

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    bool found = false;
    bool stopped = false;

    if (Process32First(snapshot, &pe)) {
        do {
            std::string exe = toLower(pe.szExeFile);

            if (exe == target) {
                found = true;

                HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (h) {
                    if (TerminateProcess(h, 0)) {
                        stopped = true;
                    }
                    CloseHandle(h);
                }
            }

        } while (Process32Next(snapshot, &pe));
    }

    CloseHandle(snapshot);

    if (!found)
        return json(false, "not_found", "STOP_APP_BY_NAME");

    if (stopped)
        return json(true, "stopped " + target, "STOP_APP_BY_NAME");

    return json(false, "access_denied", "STOP_APP_BY_NAME");
}

#else

std::string cmdListApps() {
    return json(false, "not_supported", "LIST_APPS");
}

std::string cmdStart(const std::vector<std::string>&) {
    return json(false, "not_supported", "START_APP_BY_NAME");
}

std::string cmdStop(const std::vector<std::string>&) {
    return json(false, "not_supported", "STOP_APP_BY_NAME");
}

#endif
