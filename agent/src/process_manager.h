#pragma once

#include <string>
#include <vector>

std::string listProcessesJson();

std::string startProcessByName(const std::vector<std::string>& parts);

std::string killProcessByPid(const std::string& pid, const std::string& commandLabel);