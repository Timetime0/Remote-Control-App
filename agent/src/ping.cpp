#include "ping.h"
#include "utils.h"

std::string cmdPing() {
    return jsonCommandResponse(true, "pong", "PING");
}
