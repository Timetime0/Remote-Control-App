#include "process_manager.h"
#include "app_manager.h"
#include "utils.h"

#include <string>
#include <vector>

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
#if defined(_WIN32) || defined(__APPLE__)
    return startExecutableByName(joinParts(parts, 1), "START_PROCESS");
#else
    if (parts.size() < 2) {
        return "{\"ok\":false,\"command\":\"START_PROCESS\",\"message\":\"missing_name\"}";
    }

    const std::string target = joinParts(parts, 1);
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