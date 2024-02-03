#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pl::utils {

constexpr const int MAX_PATH_LENGTH = 8192;

std::wstring str2wstr(const std::string& str, unsigned int codePage);

std::wstring str2wstr(const std::string& str);

std::string wstr2str(const std::wstring& wstr);

[[nodiscard]] inline std::u8string str2u8str(std::string str) {
    std::u8string& tmp = *reinterpret_cast<std::u8string*>(&str);
    return {std::move(tmp)};
}

[[nodiscard]] inline std::string u8str2str(std::u8string str) {
    std::string& tmp = *reinterpret_cast<std::string*>(&str);
    return {std::move(tmp)};
}

[[nodiscard]] inline std::u8string_view sv2u8sv(std::string_view sv) {
    return {reinterpret_cast<const char8_t*>(sv.data()), sv.size()};
}

[[nodiscard]] inline std::string_view u8sv2sv(std::u8string_view sv) {
    return {reinterpret_cast<const char*>(sv.data()), sv.size()};
}

std::vector<std::string_view> split(std::string_view s, std::string_view delimiter);

} // namespace pl::utils
