#include "mouse_control.h"

#ifdef _WIN32
#include <windows.h>
#endif

std::string mouseMoveJson(int x, int y) {
#ifdef _WIN32
    SetCursorPos(x, y);

    return "{"
        "\"ok\":true,"
        "\"command\":\"MOUSE_MOVE\","
        "\"message\":\"mouse_moved\""
        "}";
#else
    return "{\"ok\":false,\"command\":\"MOUSE_MOVE\",\"message\":\"not_supported\"}";
#endif
}

std::string mouseLeftClickJson(int x, int y) {
#ifdef _WIN32
    SetCursorPos(x, y);

    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

    return "{"
        "\"ok\":true,"
        "\"command\":\"MOUSE_LEFT_CLICK\","
        "\"message\":\"left_click_done\""
        "}";
#else
    return "{\"ok\":false,\"command\":\"MOUSE_LEFT_CLICK\",\"message\":\"not_supported\"}";
#endif
}

std::string mouseRightClickJson(int x, int y) {
#ifdef _WIN32
    SetCursorPos(x, y);

    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);

    return "{"
        "\"ok\":true,"
        "\"command\":\"MOUSE_RIGHT_CLICK\","
        "\"message\":\"right_click_done\""
        "}";
#else
    return "{\"ok\":false,\"command\":\"MOUSE_RIGHT_CLICK\",\"message\":\"not_supported\"}";
#endif
}