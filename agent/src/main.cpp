#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketType = SOCKET;
constexpr SocketType INVALID_SOCKET_FD = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketType = int;
constexpr SocketType INVALID_SOCKET_FD = -1;
#endif

namespace {

  std::string trim(const std::string& input) {
    const auto start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(start, end - start + 1);
  }

  std::vector<std::string> splitCommand(const std::string& line) {
    std::istringstream stream(line);
    std::vector<std::string> parts;
    std::string token;
    while (stream >> token) {
      parts.push_back(token);
    }
    return parts;
  }

  std::string escapeJson(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
      switch (c) {
        case '\\':
          out += "\\\\";
          break;
        case '"':
          out += "\\\"";
          break;
        case '\n':
          out += "\\n";
          break;
        case '\r':
          out += "\\r";
          break;
        case '\t':
          out += "\\t";
          break;
        default:
          out += c;
          break;
      }
    }
    return out;
  }

  std::string runShellCommand(const std::string& command) {
  #ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
  #else
    FILE* pipe = popen(command.c_str(), "r");
  #endif
    if (!pipe) {
      return "shell_error";
    }

    std::array<char, 512> buffer{};
    std::string output;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
      output += buffer.data();
      if (output.size() > 3000) {
        output = output.substr(0, 3000) + "\n...[truncated]";
        break;
      }
    }

  #ifdef _WIN32
    _pclose(pipe);
  #else
    pclose(pipe);
  #endif
    return trim(output.empty() ? "empty_output" : output);
  }

  std::string listProcesses() {
  #ifdef _WIN32
    return runShellCommand("tasklist /FO TABLE");
  #else
    return runShellCommand("ps -axo pid,comm | head -n 20");
  #endif
  }

  std::string commandNotImplemented(const std::string& cmd) {
    return "{\"ok\":false,\"command\":\"" + escapeJson(cmd) +
          "\",\"message\":\"not_implemented_yet\"}";
  }

  std::string processCommand(const std::string& rawLine) {
    const auto line = trim(rawLine);
    const auto parts = splitCommand(line);
    if (parts.empty()) {
      return "{\"ok\":false,\"message\":\"empty_command\"}";
    }

    const std::string& cmd = parts[0];
    if (cmd == "PING") {
      return "{\"ok\":true,\"message\":\"pong\"}";
    }
    if (cmd == "LIST_PROCESSES") {
      return "{\"ok\":true,\"command\":\"LIST_PROCESSES\",\"output\":\"" +
            escapeJson(listProcesses()) + "\"}";
    }
    if (cmd == "SCREENSHOT") {
      return commandNotImplemented(cmd + " (hook native API)");
    }
    if (cmd == "LIST_APPS" || cmd == "START_APP" || cmd == "STOP_APP" ||
        cmd == "KILL_PROCESS" || cmd == "KEYLOGGER_START" || cmd == "KEYLOGGER_STOP" ||
        cmd == "COPY_FILE" || cmd == "SHUTDOWN" || cmd == "RESTART" ||
        cmd == "WEBCAM_START" || cmd == "WEBCAM_RECORD_START" || cmd == "WEBCAM_RECORD_STOP") {
      return commandNotImplemented(cmd);
    }

    return "{\"ok\":false,\"message\":\"unknown_command\",\"command\":\"" + escapeJson(cmd) +
          "\"}";
  }

  void closeSocket(SocketType fd) {
  #ifdef _WIN32
    closesocket(fd);
  #else
    close(fd);
  #endif
  }

  void handleClient(SocketType clientFd) {
    std::array<char, 1024> buffer{};
    std::string data;

    while (true) {
  #ifdef _WIN32
      const int bytesRead = recv(clientFd, buffer.data(), static_cast<int>(buffer.size()), 0);
  #else
      const int bytesRead = static_cast<int>(recv(clientFd, buffer.data(), buffer.size(), 0));
  #endif
      if (bytesRead <= 0) break;
      data.append(buffer.data(), bytesRead);
      auto newlinePos = data.find('\n');
      if (newlinePos != std::string::npos) {
        const std::string line = data.substr(0, newlinePos);
        const std::string response = processCommand(line) + "\n";
  #ifdef _WIN32
        send(clientFd, response.c_str(), static_cast<int>(response.size()), 0);
  #else
        send(clientFd, response.c_str(), response.size(), 0);
  #endif
        break;
      }
    }

    closeSocket(clientFd);
  }

  bool initSockets() {
  #ifdef _WIN32
    WSADATA wsData{};
    return WSAStartup(MAKEWORD(2, 2), &wsData) == 0;
  #else
    return true;
  #endif
  }

  void shutdownSockets() {
  #ifdef _WIN32
    WSACleanup();
  #endif
  }

}  // namespace

int main(int argc, char* argv[]) {
  int port = 5050;
  if (argc >= 2) {
    port = std::atoi(argv[1]);
    if (port <= 0) {
      std::cerr << "Invalid port.\n";
      return 1;
    }
  }

  if (!initSockets()) {
    std::cerr << "Socket init failed.\n";
    return 1;
  }

  SocketType serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd == INVALID_SOCKET_FD) {
    std::cerr << "Cannot create socket.\n";
    shutdownSockets();
    return 1;
  }

  int opt = 1;
#ifdef _WIN32
  setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
  setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(static_cast<uint16_t>(port));

  if (bind(serverFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
    std::cerr << "Bind failed on port " << port << ".\n";
    closeSocket(serverFd);
    shutdownSockets();
    return 1;
  }

  if (listen(serverFd, 16) < 0) {
    std::cerr << "Listen failed.\n";
    closeSocket(serverFd);
    shutdownSockets();
    return 1;
  }

  std::cout << "Remote agent listening on port " << port << '\n';

  while (true) {
    sockaddr_in clientAddr{};
#ifdef _WIN32
    int clientLen = sizeof(clientAddr);
    SocketType clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
#else
    socklen_t clientLen = sizeof(clientAddr);
    SocketType clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
#endif
    if (clientFd == INVALID_SOCKET_FD) {
      continue;
    }
    std::thread worker(handleClient, clientFd);
    worker.detach();
  }

  closeSocket(serverFd);
  shutdownSockets();
  return 0;
}
