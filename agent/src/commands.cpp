#include "commands.h"

#include "ping.h"
#include "screenshot.h"
#include "app_manager.h"
#include "process_manager.h"

#include <sstream>
#include <vector>
#include <string>

// ===== UTILS =====
static std::string trim(const std::string& input) {
    const auto start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(start, end - start + 1);
}

static std::vector<std::string> split(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> out;
    std::string t;
    while (iss >> t) out.push_back(t);
    return out;
}

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

static std::string json(bool ok, const std::string& msg, const std::string& cmd = "") {
    return "{\"ok\":" + std::string(ok ? "true" : "false") +
        ",\"command\":\"" + cmd +
        "\",\"message\":\"" + escapeJson(msg) + "\"}";
}

// ===== ROUTER =====
std::string process(const std::string& line) {
    auto parts = split(trim(line));
    if (parts.empty())
        return json(false, "empty");

    const std::string& cmd = parts[0];

    // ===== PING =====
    if (cmd == "PING") return cmdPing();

    // ===== SCREENSHOT =====
    if (cmd == "SCREENSHOT") {
        return captureScreenshotJson();
    }

    // ===== APP MANAGER =====
    if (cmd == "LIST_APPS") return cmdListApps();
    if (cmd == "START_APP_BY_NAME") return cmdStart(parts);
    if (cmd == "STOP_APP_BY_NAME") return cmdStop(parts);

    // ===== PROCESS MANAGER =====
    if (cmd == "LIST_PROCESSES") return listProcessesJson();

    if (cmd == "START_PROCESS") return startProcessByName(parts);

    if (cmd == "KILL_PROCESS" && parts.size() >= 2)
        return killProcessByPid(parts[1], "KILL_PROCESS");

    if (cmd == "STOP_APP_BY_PID" && parts.size() >= 2)
        return killProcessByPid(parts[1], "STOP_APP_BY_PID");

    // ===== UNKNOWN =====
    return json(false, "unknown_command", cmd);
}
