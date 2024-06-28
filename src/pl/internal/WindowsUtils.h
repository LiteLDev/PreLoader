#pragma once

#include <ctime>
#include <filesystem>
#include <string>
#include <utility>

#include <intrin.h>

namespace pl::utils {

std::optional<std::filesystem::path> getModulePath(void* handle, void* process = nullptr);

void* getModuleHandle(void* addr);

std::string getModuleFileName(void* handle, void* process = nullptr);

[[nodiscard]] inline std::string getCallerModuleFileName(void* addr = _ReturnAddress()) {
    return getModuleFileName(getModuleHandle(addr));
}
std::pair<std::tm, int> getLocalTime();

std::filesystem::path getSystemRoot();

} // namespace pl::utils
