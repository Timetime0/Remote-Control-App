#include "mouse_control.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#endif

std::string mouseMoveJson(int x, int y) {
#ifdef _WIN32
    SetCursorPos(x, y);

    return "{"
        "\"ok\":true,"
        "\"command\":\"MOUSE_MOVE\","
        "\"message\":\"mouse_moved\""
        "}";
#elif defined(__APPLE__)
    const CGPoint point = CGPointMake(static_cast<CGFloat>(x), static_cast<CGFloat>(y));
    CGEventRef moveEvent = CGEventCreateMouseEvent(
        nullptr, kCGEventMouseMoved, point, kCGMouseButtonLeft
    );
    if (!moveEvent) {
        return "{\"ok\":false,\"command\":\"MOUSE_MOVE\",\"message\":\"event_create_failed\"}";
    }
    CGEventPost(kCGHIDEventTap, moveEvent);
    CFRelease(moveEvent);
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
#elif defined(__APPLE__)
    const CGPoint point = CGPointMake(static_cast<CGFloat>(x), static_cast<CGFloat>(y));

    CGEventRef moveEvent = CGEventCreateMouseEvent(
        nullptr, kCGEventMouseMoved, point, kCGMouseButtonLeft
    );
    CGEventRef downEvent = CGEventCreateMouseEvent(
        nullptr, kCGEventLeftMouseDown, point, kCGMouseButtonLeft
    );
    CGEventRef upEvent = CGEventCreateMouseEvent(
        nullptr, kCGEventLeftMouseUp, point, kCGMouseButtonLeft
    );

    if (!moveEvent || !downEvent || !upEvent) {
        if (moveEvent) CFRelease(moveEvent);
        if (downEvent) CFRelease(downEvent);
        if (upEvent) CFRelease(upEvent);
        return "{\"ok\":false,\"command\":\"MOUSE_LEFT_CLICK\",\"message\":\"event_create_failed\"}";
    }

    CGEventPost(kCGHIDEventTap, moveEvent);
    CGEventPost(kCGHIDEventTap, downEvent);
    CGEventPost(kCGHIDEventTap, upEvent);

    CFRelease(moveEvent);
    CFRelease(downEvent);
    CFRelease(upEvent);

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
#elif defined(__APPLE__)
    const CGPoint point = CGPointMake(static_cast<CGFloat>(x), static_cast<CGFloat>(y));

    CGEventRef moveEvent = CGEventCreateMouseEvent(
        nullptr, kCGEventMouseMoved, point, kCGMouseButtonRight
    );
    CGEventRef downEvent = CGEventCreateMouseEvent(
        nullptr, kCGEventRightMouseDown, point, kCGMouseButtonRight
    );
    CGEventRef upEvent = CGEventCreateMouseEvent(
        nullptr, kCGEventRightMouseUp, point, kCGMouseButtonRight
    );

    if (!moveEvent || !downEvent || !upEvent) {
        if (moveEvent) CFRelease(moveEvent);
        if (downEvent) CFRelease(downEvent);
        if (upEvent) CFRelease(upEvent);
        return "{\"ok\":false,\"command\":\"MOUSE_RIGHT_CLICK\",\"message\":\"event_create_failed\"}";
    }

    CGEventPost(kCGHIDEventTap, moveEvent);
    CGEventPost(kCGHIDEventTap, downEvent);
    CGEventPost(kCGHIDEventTap, upEvent);

    CFRelease(moveEvent);
    CFRelease(downEvent);
    CFRelease(upEvent);

    return "{"
        "\"ok\":true,"
        "\"command\":\"MOUSE_RIGHT_CLICK\","
        "\"message\":\"right_click_done\""
        "}";
#else
    return "{\"ok\":false,\"command\":\"MOUSE_RIGHT_CLICK\",\"message\":\"not_supported\"}";
#endif
}