#pragma once

#include <string>

namespace pl::utils {

std::wstring str2wstr(const std::string& str, unsigned int codePage);

std::wstring str2wstr(const std::string& str);

std::string wstr2str(const std::wstring& wstr);
} // namespace pl::utils
