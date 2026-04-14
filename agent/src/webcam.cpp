#include "webcam.h"

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

static std::string escapeJson(const std::string& input) {
    std::string out;
    for (char c : input) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        default: out += c;
        }
    }
    return out;
}

std::string startWebcamJson() {
#ifdef _WIN32
    return "{"
        "\"ok\":true,"
        "\"command\":\"WEBCAM_START\","
        "\"message\":\"webcam_started\""
        "}";
#else
    return "{"
        "\"ok\":false,"
        "\"command\":\"WEBCAM_START\","
        "\"message\":\"not_supported\""
        "}";
#endif
}

std::string startWebcamRecordJson() {
#ifdef _WIN32
    return "{"
        "\"ok\":true,"
        "\"command\":\"WEBCAM_RECORD_START\","
        "\"message\":\"record_started\""
        "}";
#else
    return "{"
        "\"ok\":false,"
        "\"command\":\"WEBCAM_RECORD_START\","
        "\"message\":\"not_supported\""
        "}";
#endif
}

std::string stopWebcamRecordJson() {
#ifdef _WIN32
    return "{"
        "\"ok\":true,"
        "\"command\":\"WEBCAM_RECORD_STOP\","
        "\"message\":\"record_stopped\""
        "}";
#else
    return "{"
        "\"ok\":false,"
        "\"command\":\"WEBCAM_RECORD_STOP\","
        "\"message\":\"not_supported\""
        "}";
#endif
}