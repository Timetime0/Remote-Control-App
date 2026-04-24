#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "commands.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
using SocketType = SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketType = int;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#endif

#ifdef _WIN32
static void closeSock(SocketType s) { closesocket(s); }
#else
static void closeSock(SocketType s) { close(s); }
#endif

static bool init() {
#ifdef _WIN32
    WSADATA wsa{};
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return true;
#endif
}

static void cleanupNet() {
#ifdef _WIN32
    WSACleanup();
#endif
}

static void sendAll(SocketType client, const std::string& data) {
    size_t total = 0;
    while (total < data.size()) {
        int sent = send(
            client,
            data.c_str() + total,
            static_cast<int>(data.size() - total),
            0
        );
        if (sent <= 0) return;
        total += static_cast<size_t>(sent);
    }
}

static void handle(SocketType client) {
    std::array<char, 4096> buf{};
    std::string data;

    while (true) {
        int n = recv(client, buf.data(), static_cast<int>(buf.size()), 0);
        if (n == 0) break;
        if (n < 0) break;

        data.append(buf.data(), static_cast<size_t>(n));

        size_t pos;
        while ((pos = data.find('\n')) != std::string::npos) {
            std::string line = data.substr(0, pos);
            data.erase(0, pos + 1);

            std::string res = process(line, client);
            res += "\n<<END>>\n";
            sendAll(client, res);
        }
    }

    closeSock(client);
}

int main() {
    if (!init()) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SocketType serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        std::cerr << "Cannot create socket\n";
        cleanupNet();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5050);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        closeSock(serverSock);
        cleanupNet();
        return 1;
    }

    if (listen(serverSock, 5) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        closeSock(serverSock);
        cleanupNet();
        return 1;
    }

    std::cout << "Agent listening on port 5050...\n";

    while (true) {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);

        SocketType client = accept(
            serverSock,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientLen
        );

        if (client == INVALID_SOCKET) {
            continue;
        }

        std::thread(handle, client).detach();
    }

    closeSock(serverSock);
    cleanupNet();
    return 0;
}