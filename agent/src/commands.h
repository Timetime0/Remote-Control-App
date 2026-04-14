#pragma once
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
using SocketType = SOCKET;
#else
using SocketType = int;
#endif

// Xu ly command tu socket
std::string process(const std::string& line, SocketType client);