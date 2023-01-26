#include "utils.h"

#include <filesystem>

#include <Windows.h>

#include <Psapi.h>

std::string pl::utils::GetCallerModuleFileName(unsigned long FramesToSkip) {
    static const int maxFrameCount = 1;
    void* frames[maxFrameCount];
    int frameCount = CaptureStackBackTrace(FramesToSkip + 2, maxFrameCount, frames, nullptr);

    HANDLE hProcess = GetCurrentProcess();

    if (0 < frameCount) {
        HMODULE hModule;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (LPCWSTR)frames[0], &hModule);
        wchar_t buf[MAX_PATH] = {0};
        GetModuleFileNameEx(GetCurrentProcess(), hModule, buf, MAX_PATH);
        return std::filesystem::path(buf).filename().u8string();
    }
    return "Unknown";
}

std::wstring pl::utils::str2wstr(const std::string& str, UINT codePage) {
    auto len = MultiByteToWideChar(codePage, 0, str.c_str(), -1, nullptr, 0);
    auto* buffer = new wchar_t[len];
    MultiByteToWideChar(codePage, 0, str.c_str(), -1, buffer, len);
    std::wstring result = std::wstring(buffer);
    delete[] buffer;
    return result;
}

std::wstring pl::utils::str2wstr(const std::string& str) { return str2wstr(str, CP_UTF8); }

std::string pl::utils::wstr2str(const std::wstring& wstr) {
    auto len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    char* buffer = new char[len];
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buffer, len, nullptr, nullptr);
    std::string result = std::string(buffer);
    delete[] buffer;
    return result;
}