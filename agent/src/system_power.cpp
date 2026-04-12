#include "system_power.h"

#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

static std::string trim(const std::string& input) {
    const auto start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(start, end - start + 1);
}

static std::string escapeJson(const std::string& input) {
    std::string esc;
    for (char c : input) {
        if (c == '\\') esc += "\\\\";
        else if (c == '"') esc += "\\\"";
        else if (c == '\n') esc += "\\n";
        else esc += c;
    }
    return esc;
}

static std::string runShellCommand(const std::string& command) {
#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (!pipe) return "shell_error";

    char buffer[512];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return trim(output);
}

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
