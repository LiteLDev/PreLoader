#include "pl/internal/StringUtils.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <windows.h>

#include <minwindef.h>
#include <stringapiset.h>
#include <winnls.h>

std::wstring pl::utils::str2wstr(const std::string& str, UINT codePage) {
    int          len = MultiByteToWideChar(codePage, 0, str.data(), (int)str.size(), nullptr, 0);
    std::wstring wstr;
    if (len == 0) return wstr;
    wstr.resize(len);
    MultiByteToWideChar(codePage, 0, str.data(), (int)str.size(), wstr.data(), len);
    return wstr;
}

std::wstring pl::utils::str2wstr(const std::string& str) { return str2wstr(str, CP_UTF8); }

std::string pl::utils::wstr2str(const std::wstring& wstr) {
    int         len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string str;
    if (len == 0) return str;
    str.resize(len);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), str.data(), (int)str.size(), nullptr, nullptr);
    return str;
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
