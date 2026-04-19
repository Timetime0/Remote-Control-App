#include "app_manager.h"
#include "utils.h"

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <set>
#include <cstdio>
#include <cstdlib>

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

#ifdef _WIN32

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static std::string getFileName(const std::string& path) {
    size_t pos = path.find_last_of("\\/");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
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

    if (a.find(b) != std::string::npos) return true;

    int j = 0;
    for (char c : a) {
        if (j < b.size() && c == b[j]) j++;
    }
    return j == b.size();
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

struct AppInfo {
    int pid;
    std::string name;
    std::string path;
};

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

std::string cmdListApps() {
    std::vector<AppInfo> apps;
    EnumWindows(EnumWindowsProc, (LPARAM)&apps);

    std::set<std::string> seen;
    std::ostringstream oss;
    oss << "[";

    bool first = true;

    for (auto& app : apps) {
        std::string key = toLower(app.name);

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

std::string cmdStart(const std::vector<std::string>& parts) {
    const std::string target = joinParts(parts, 1);
    if (target.empty())
        return jsonCommandResponse(false, "missing_app", "START_APP_BY_NAME");

    std::string alias = resolveAlias(target);
    if (!alias.empty()) {
        HINSTANCE res = ShellExecuteA(NULL, "open", alias.c_str(), NULL, NULL, SW_SHOWNORMAL);
        if ((INT_PTR)res > 32)
            return jsonCommandResponse(true, "started " + alias, "START_APP_BY_NAME");
    }

    std::string tryExe = target;
    if (tryExe.find(".exe") == std::string::npos)
        tryExe += ".exe";

    HINSTANCE res = ShellExecuteA(NULL, "open", tryExe.c_str(), NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)res > 32)
        return jsonCommandResponse(true, "started " + tryExe, "START_APP_BY_NAME");

    buildCache();

    for (const auto& app : g_cache) {
        if (match(app.name, target)) {
            HINSTANCE r = ShellExecuteA(NULL, "open", app.path.c_str(), NULL, NULL, SW_SHOWNORMAL);

            if ((INT_PTR)r > 32)
                return jsonCommandResponse(true, "started " + app.name, "START_APP_BY_NAME");
        }
    }

    return jsonCommandResponse(false, "not_found", "START_APP_BY_NAME");
}

std::string cmdStop(const std::vector<std::string>& parts) {
    std::string target = joinParts(parts, 1);
    if (target.empty())
        return jsonCommandResponse(false, "missing_name", "STOP_APP_BY_NAME");

    target = toLower(target);

    if (target.find(".exe") == std::string::npos)
        target += ".exe";

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return jsonCommandResponse(false, "snapshot_fail", "STOP_APP_BY_NAME");

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
        return jsonCommandResponse(false, "not_found", "STOP_APP_BY_NAME");

    if (stopped)
        return jsonCommandResponse(true, "stopped " + target, "STOP_APP_BY_NAME");

    return jsonCommandResponse(false, "access_denied", "STOP_APP_BY_NAME");
}

#elif defined(__APPLE__)

static std::string stripAppBundleSuffix(const std::string& s) {
    const std::string suffix = ".app";
    if (s.size() > suffix.size() &&
        s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0) {
        return s.substr(0, s.size() - suffix.size());
    }
    return s;
}

static std::string escapeAppleScriptString(const std::string& input) {
    std::string out;
    for (char c : input) {
        if (c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
}

static std::string escapeForDoubleQuotedShell(const std::string& input) {
    std::string out;
    for (char c : input) {
        if (c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
}

static std::string shellSingleQuote(const std::string& s) {
    std::string r = "'";
    for (char c : s) {
        if (c == '\'') r += "'\\''";
        else r += c;
    }
    r += "'";
    return r;
}

static bool isAppRunning(const std::string& appLine) {
    const std::string pattern = "/" + appLine + "/";
    const std::string cmd = "pgrep -f " + shellSingleQuote(pattern) + " 2>/dev/null | head -1";
    const std::string out = runShellCommand(cmd);
    return !out.empty() && out != "shell_error";
}

static std::string listApplicationsDirListing() {
    return runShellCommand("ls /Applications 2>/dev/null");
}

std::string cmdListApps() {
    const std::string list = listApplicationsDirListing();
    if (list.empty() || list == "shell_error") {
        return "{\"ok\":false,\"command\":\"LIST_APPS\",\"message\":\"" +
            escapeJson("list_failed") + "\",\"items\":[]}";
    }

    const std::vector<std::string> lines = splitLines(list);
    std::ostringstream oss;
    oss << "[";

    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) oss << ",";
        const std::string& bundle = lines[i];
        const bool running = isAppRunning(bundle);
        const std::string path = "/Applications/" + bundle;
        const std::string display = stripAppBundleSuffix(bundle);

        oss << "{"
            << "\"name\":\"" << escapeJson(display) << "\","
            << "\"running\":" << (running ? "true" : "false") << ","
            << "\"raw\":\"" << escapeJson(bundle) << "\","
            << "\"path\":\"" << escapeJson(path) << "\""
            << "}";
    }

    oss << "]";
    return "{\"ok\":true,\"command\":\"LIST_APPS\",\"items\":" + oss.str() + "}";
}

std::string cmdStart(const std::vector<std::string>& parts) {
    const std::string appName = joinParts(parts, 1);
    if (appName.empty())
        return jsonCommandResponse(false, "missing_app", "START_APP_BY_NAME");

    const std::string out =
        runShellCommand("open -a \"" + escapeForDoubleQuotedShell(appName) + "\" 2>&1");
    const bool shellFailed = (out == "shell_error");
    if (shellFailed) {
        return jsonCommandResponse(false, "shell_failed", "START_APP_BY_NAME");
    }
    if (!out.empty() && out.find("Unable to find") != std::string::npos) {
        return jsonCommandResponse(false, out, "START_APP_BY_NAME");
    }
    return jsonCommandResponse(true, out.empty() ? "started" : out, "START_APP_BY_NAME");
}

std::string cmdStop(const std::vector<std::string>& parts) {
    const std::string appName = joinParts(parts, 1);
    if (appName.empty())
        return jsonCommandResponse(false, "missing_name", "STOP_APP_BY_NAME");

    const std::string display = stripAppBundleSuffix(appName);
    const std::string out = runShellCommand(
        "osascript -e \"tell application \\\"" + escapeAppleScriptString(display) +
        "\\\" to quit\" 2>&1");

    const bool shellFailed = (out == "shell_error");
    if (shellFailed) {
        return jsonCommandResponse(false, "shell_failed", "STOP_APP_BY_NAME");
    }
    const bool err =
        out.find("error") != std::string::npos ||
        out.find("Error") != std::string::npos ||
        out.find("execution error") != std::string::npos;
    if (err) {
        return jsonCommandResponse(false, out.empty() ? "quit_failed" : out, "STOP_APP_BY_NAME");
    }
    return jsonCommandResponse(true, out.empty() ? "quit_requested" : out, "STOP_APP_BY_NAME");
}

#else

std::string cmdListApps() {
    return jsonCommandResponse(false, "not_supported", "LIST_APPS");
}

std::string cmdStart(const std::vector<std::string>&) {
    return jsonCommandResponse(false, "not_supported", "START_APP_BY_NAME");
}

std::string cmdStop(const std::vector<std::string>&) {
    return jsonCommandResponse(false, "not_supported", "STOP_APP_BY_NAME");
}

#endif
