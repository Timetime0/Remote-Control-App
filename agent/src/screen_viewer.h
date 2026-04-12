#pragma once

#include <string>

#ifdef _WIN32
#include <winsock2.h>
using SocketType = SOCKET;
#else
using SocketType = int;
#endif

void startScreenViewer(SocketType client);
void stopScreenViewer();
bool isScreenViewerRunning();