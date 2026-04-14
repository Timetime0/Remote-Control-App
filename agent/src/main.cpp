#include "commands.h"
#include "screen_viewer.h"

#include <cstdlib>
#include <iostream>
#include <thread>
#include <array>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
using SocketType = SOCKET;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
using SocketType = int;
#endif

// ===== CLOSE SOCKET =====
void closeSock(SocketType s) {
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

// ===== SENDALL =====
void sendAll(SocketType sock, const std::string& data) {
    size_t total = 0;

    while (total < data.size()) {
        int sent = send(sock, data.c_str() + total, data.size() - total, 0);

        if (sent <= 0) {
            return;
        }

        total += sent;
    }
}

// ===== HANDLE CLIENT =====
void handle(SocketType client) {
    std::array<char, 1024> buf{};
    std::string data;

    while (true) {
        int n = recv(client, buf.data(), buf.size(), 0);

        if (n == 0) break;
        if (n < 0) break;

        data.append(buf.data(), n);

        size_t pos;
        while ((pos = data.find('\n')) != std::string::npos) {
            std::string line = data.substr(0, pos);
            data.erase(0, pos + 1);

            std::string res = process(line, client) + "\n<<END>>\n";
            sendAll(client, res);
        }
    }

    closeSock(client);
}

// ===== INIT SOCKET =====
bool init() {
#ifdef _WIN32
    WSADATA d;
    return WSAStartup(MAKEWORD(2, 2), &d) == 0;
#else
    return true;
#endif
}

// ===== CLEANUP =====
void shutdownSock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// ===== MAIN =====
int main(int argc, char** argv) {
    if (!init()) {
        std::cerr << "Socket init failed\n";
        return 1;
    }

    int port = 5050;
    if (argc >= 2) {
        const int p = std::atoi(argv[1]);
        if (p > 0 && p <= 65535) {
            port = p;
        } else {
            std::cerr << "Invalid port, using 5050\n";
        }
    }

    SocketType server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        std::cerr << "Cannot create socket\n";
        return 1;
    }

    // fix loi restart server bi chiem port
    int opt = 1;
#ifdef _WIN32
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(port));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    if (listen(server, 10) < 0) {
        std::cerr << "Listen failed\n";
        return 1;
    }

    std::cout << "Server running on port " << port << "...\n";

    while (true) {
        sockaddr_in client{};
#ifdef _WIN32
        int len = sizeof(client);
#else
        socklen_t len = sizeof(client);
#endif

        SocketType c = accept(server, (sockaddr*)&client, &len);
        if (c < 0) continue;

        std::thread(handle, c).detach();
    }

    closeSock(server);
    shutdownSock();
    return 0;
}
