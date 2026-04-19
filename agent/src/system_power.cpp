#include "system_power.h"
#include "utils.h"

#include <string>

std::string shutdownOrRestartJson(const std::string& commandLabel) {
#if defined(_WIN32) || defined(__APPLE__)
    std::string out;
#ifdef _WIN32
    if (commandLabel == "SHUTDOWN") {
        out = runShellCommand("shutdown /s /f /t 0 2>&1");
    } else {
        out = runShellCommand("shutdown /r /f /t 0 2>&1");
    }
#else
    if (commandLabel == "SHUTDOWN") {
        out = runShellCommand(
            "osascript -e 'tell application \"System Events\" to shut down' 2>&1");
    } else {
        out = runShellCommand(
            "osascript -e 'tell application \"System Events\" to restart' 2>&1");
    }
#endif
    const bool shellFailed = (out == "shell_error");
    if (shellFailed) {
        return "{\"ok\":false,\"command\":\"" + escapeJson(commandLabel) +
               "\",\"message\":\"shell_failed\",\"output\":\"\"}";
    }
    return "{\"ok\":true,\"command\":\"" + escapeJson(commandLabel) + "\",\"output\":\"" +
           escapeJson(out) + "\"}";
#else
    (void)commandLabel;
    return "{\"ok\":false,\"command\":\"SHUTDOWN\",\"message\":\"not_supported\"}";
#endif
}
