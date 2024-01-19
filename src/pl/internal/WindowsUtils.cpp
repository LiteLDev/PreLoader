#include "pl/internal/WindowsUtils.h"
#include "StringUtils.h"

#include <filesystem>

#include <Windows.h>

#include <Psapi.h>

namespace pl::utils {
std::string GetCallerModuleFileName(unsigned long FramesToSkip) {
    static const int maxFrameCount = 1;

    void* frames[maxFrameCount];
    int   frameCount = CaptureStackBackTrace(FramesToSkip + 2, maxFrameCount, frames, nullptr);

    if (0 < frameCount) {
        HMODULE hModule;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)frames[0],
            &hModule
        );
        wchar_t buf[MAX_PATH_LENGTH] = {0};
        GetModuleFileNameExW(GetCurrentProcess(), hModule, buf, MAX_PATH_LENGTH);
        return u8str2str(std::filesystem::path(buf).filename().u8string());
    }
    return "Unknown";
}

std::pair<std::tm, int> getLocalTime() {
    SYSTEMTIME sysTime;
    GetLocalTime(&sysTime);
    std::tm time{
        .tm_sec   = sysTime.wSecond,      // seconds after the minute - [0, 60] including leap second
        .tm_min   = sysTime.wMinute,      // minutes after the hour - [0, 59]
        .tm_hour  = sysTime.wHour,        // hours since midnight - [0, 23]
        .tm_mday  = sysTime.wDay,         // day of the month - [1, 31]
        .tm_mon   = sysTime.wMonth - 1,   // months since January - [0, 11]
        .tm_year  = sysTime.wYear - 1900, // years since 1900
        .tm_wday  = sysTime.wDayOfWeek,   // days since Sunday - [0, 6]
        .tm_isdst = -1                    // daylight savings time flag
    };
    return {time, sysTime.wMilliseconds};
}
} // namespace pl::utils
