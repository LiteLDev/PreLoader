#pragma once

#include <ctime>
#include <filesystem>
#include <string>
#include <utility>

namespace pl::utils {

std::string GetCallerModuleFileName(unsigned long FramesToSkip = 0);

std::pair<std::tm, int> getLocalTime();

std::filesystem::path getSystemRoot();

} // namespace pl::utils
