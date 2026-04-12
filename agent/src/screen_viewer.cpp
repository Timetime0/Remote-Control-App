#include "screen_viewer.h"
#include "screenshot.h"

#include <thread>
#include <atomic>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#endif

static std::atomic<bool> g_running(false);

void startScreenViewer(SocketType client) {
    if (g_running) return;

    g_running = true;

    std::thread([client]() {
        while (g_running) {
            std::string frame = captureScreenshotJson();

            const std::string from = "\"command\":\"SCREENSHOT\"";
            const std::string to = "\"command\":\"SCREEN_VIEWER_FRAME\"";

            const auto pos = frame.find(from);
            if (pos != std::string::npos) {
                frame.replace(pos, from.size(), to);
            }
            std::string msg = frame + "\n<<END>>\n";

            send(client, msg.c_str(), static_cast<int>(msg.size()), 0);

#ifdef _WIN32
            Sleep(1000);
            //ULONG quality = 90;
#else
            sleep(1);
#endif
        }
        }).detach();
}

void stopScreenViewer() {
    g_running = false;
}

bool isScreenViewerRunning() {
    return g_running;
}