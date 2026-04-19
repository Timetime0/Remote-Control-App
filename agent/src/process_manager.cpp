#include "process_manager.h"
#include "utils.h"

#include <algorithm>
#include <sstream>
#include <utility>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// ===== LIST PROCESSES =====
std::string listProcessesJson() {
#ifdef _WIN32
    const std::string output = runShellCommand(
        "powershell -NoProfile -Command \"Get-Process | "
        "Select-Object Id, ProcessName | Format-Table -HideTableHeaders\""
    );
#else
    const std::string output = runShellCommand("ps -axo pid,comm");
#endif

    return "{\"ok\":true,\"command\":\"LIST_PROCESSES\",\"output\":\"" +
        escapeJson(output) + "\"}";
}

// ===== START PROCESS =====
std::string startProcessByName(const std::vector<std::string>& parts) {
    if (parts.size() < 2) {
        return "{\"ok\":false,\"command\":\"START_PROCESS\",\"message\":\"missing_name\"}";
    }

    std::string target = joinParts(parts, 1);

#ifdef _WIN32
    // Remove .exe neu co
    if (target.size() > 4 && toLower(target.substr(target.size() - 4)) == ".exe") {
        target = target.substr(0, target.size() - 4);
    }

    std::string lower = toLower(target);

    // Map app pho bien
    if (lower == "paint") target = "mspaint";
    else if (lower == "calculator") target = "calc";
    else if (lower == "calc") target = "calc";
    else if (lower == "notepad") target = "notepad";
    else if (lower == "chrome") target = "chrome";
    else if (lower == "edge") target = "msedge";
    else if (lower == "vscode") target = "code";
    else if (lower == "word") target = "winword";

    HINSTANCE result = ShellExecuteA(
        NULL,
        "open",
        target.c_str(),
        NULL,
        NULL,
        SW_SHOWNORMAL
    );

    if ((INT_PTR)result <= 32) {
        return "{\"ok\":false,\"command\":\"START_PROCESS\",\"message\":\"cannot_start\"}";
    }

    return "{\"ok\":true,\"command\":\"START_PROCESS\",\"message\":\"started " +
        escapeJson(target) + "\"}";
#else
    const std::string output = runShellCommand(target + " &");
    return "{\"ok\":true,\"command\":\"START_PROCESS\",\"output\":\"" +
        escapeJson(output) + "\"}";
#endif
}

// ===== KILL PROCESS =====
std::string killProcessByPid(const std::string& pid, const std::string& commandLabel) {
#ifdef _WIN32
    const std::string out = runShellCommand("taskkill /PID " + pid + " /F 2>&1");
#else
    const std::string out = runShellCommand("kill -9 " + pid + " 2>&1");
#endif

    const bool success =
        out.find("SUCCESS") != std::string::npos ||
        out.find("terminated") != std::string::npos ||
        out.empty();

    return "{"
        "\"ok\":" + std::string(success ? "true" : "false") + ","
        "\"command\":\"" + escapeJson(commandLabel) + "\","
        "\"message\":\"" + escapeJson(success ? "killed" : "cannot_kill") + "\","
        "\"output\":\"" + escapeJson(out) + "\""
        "}";
}