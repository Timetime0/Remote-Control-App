#include "ping.h"

// co the dung chung json util sau nay neu ban tach utils
static std::string json(bool ok, const std::string& msg, const std::string& cmd) {
    return "{\"ok\":" + std::string(ok ? "true" : "false") +
        ",\"command\":\"" + cmd +
        "\",\"message\":\"" + msg + "\"}";
}

std::string cmdPing() {
    return json(true, "pong", "PING");
}
