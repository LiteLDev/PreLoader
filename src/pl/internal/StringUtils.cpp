#include "pl/internal/StringUtils.h"

#include <Windows.h>

std::wstring pl::utils::str2wstr(const std::string& str, UINT codePage) {
    auto  len    = MultiByteToWideChar(codePage, 0, str.c_str(), -1, nullptr, 0);
    auto* buffer = new wchar_t[len];
    MultiByteToWideChar(codePage, 0, str.c_str(), -1, buffer, len);
    std::wstring result = std::wstring(buffer);
    delete[] buffer;
    return result;
}

std::wstring pl::utils::str2wstr(const std::string& str) { return str2wstr(str, CP_UTF8); }

std::string pl::utils::wstr2str(const std::wstring& wstr) {
    auto  len    = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    char* buffer = new char[len];
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buffer, len, nullptr, nullptr);
    std::string result = std::string(buffer);
    delete[] buffer;
    return result;
}

std::vector<std::string_view> pl::utils::split(std::string_view s, std::string_view delimiter) {
    size_t posStart = 0, posEnd, delimLen = delimiter.length();

    std::string_view              token;
    std::vector<std::string_view> result;

    while ((posEnd = s.find(delimiter, posStart)) != std::string::npos) {
        token    = s.substr(posStart, posEnd - posStart);
        posStart = posEnd + delimLen;
        result.push_back(token);
    }
    result.push_back(s.substr(posStart));

    return result;
}
