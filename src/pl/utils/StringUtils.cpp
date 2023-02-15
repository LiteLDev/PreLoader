#include "pl/utils/StringUtils.h"

#include <Windows.h>

std::wstring pl::utils::str2wstr(const std::string& str, UINT codePage) {
    auto  len    = MultiByteToWideChar(codePage, 0, str.c_str(), -1, nullptr, 0);
    auto* buffer = new wchar_t[len];
    MultiByteToWideChar(codePage, 0, str.c_str(), -1, buffer, len);
    std::wstring result = std::wstring(buffer);
    delete[] buffer;
    return result;
}

std::wstring pl::utils::str2wstr(const std::string& str) {
    return str2wstr(str, CP_UTF8);
}

std::string pl::utils::wstr2str(const std::wstring& wstr) {
    auto  len    = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    char* buffer = new char[len];
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buffer, len, nullptr, nullptr);
    std::string result = std::string(buffer);
    delete[] buffer;
    return result;
}
