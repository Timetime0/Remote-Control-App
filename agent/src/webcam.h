#pragma once

#include "screen_viewer.h"
#include <string>

std::string webcamStartSession(SocketType client);
void webcamStopSession();
std::string startWebcamRecordJson();
std::string stopWebcamRecordJson();
