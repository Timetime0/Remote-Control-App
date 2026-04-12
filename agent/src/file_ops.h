#pragma once
#include <string>

std::string readFileBase64Json(const std::string& path);
std::string writeFileBase64Json(const std::string& path, const std::string& dataB64);
std::string listFilesJson(const std::string& dirPath);
