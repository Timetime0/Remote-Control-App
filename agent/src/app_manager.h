#pragma once
#include <string>
#include <vector>

std::string cmdListApps();
std::string cmdStart(const std::vector<std::string>& parts);
std::string cmdStop(const std::vector<std::string>& parts);

/** Giống cmdStart nhưng trả về field command là commandLabel (vd START_PROCESS). */
std::string startExecutableByName(const std::string& target, const std::string& commandLabel);
