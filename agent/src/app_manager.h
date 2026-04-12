#pragma once
#include <string>
#include <vector>

std::string cmdListApps();
std::string cmdStart(const std::vector<std::string>& parts);
std::string cmdStop(const std::vector<std::string>& parts);
