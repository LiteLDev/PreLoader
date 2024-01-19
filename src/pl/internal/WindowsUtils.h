#pragma once

#include <string>

namespace pl::utils {

std::string GetCallerModuleFileName(unsigned long FramesToSkip = 0);

std::pair<std::tm, int> getLocalTime();
}
