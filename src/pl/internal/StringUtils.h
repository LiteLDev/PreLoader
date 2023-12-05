#pragma once

#include <string>

namespace pl::utils {

constexpr const int MAX_PATH_LENGTH = 8192;

std::wstring str2wstr(const std::string& str, unsigned int codePage);

std::wstring str2wstr(const std::string& str);

std::string wstr2str(const std::wstring& wstr);

[[nodiscard]] inline std::string u8str2str(std::u8string str) {
    std::string& tmp = *reinterpret_cast<std::string*>(&str);
    return {std::move(tmp)};
}

} // namespace pl::utils
