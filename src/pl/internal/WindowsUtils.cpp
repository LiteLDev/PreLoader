#include "pl/internal/WindowsUtils.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <string>
#include <utility>

#include "pl/internal/StringUtils.h"

#include <windows.h>

#include <Psapi.h>
#include <libloaderapi.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <processthreadsapi.h>
#include <sysinfoapi.h>
#include <winbase.h>
#include <winnt.h>

namespace pl::utils {

template <std::invocable<wchar_t*, size_t, size_t&> Fn>
[[nodiscard]] inline std::optional<std::wstring> adaptFixedSizeToAllocatedResult(Fn&& callback) noexcept {
    constexpr size_t arraySize = 256;

    wchar_t value[arraySize]{};
    size_t  valueLengthNeededWithNull{};

    std::optional<std::wstring> result{std::in_place};

    if (!std::invoke(std::forward<Fn>(callback), value, arraySize, valueLengthNeededWithNull)) {
        result.reset();
        return result;
    }
    if (valueLengthNeededWithNull <= arraySize) {
        return std::optional<std::wstring>{std::in_place, value, valueLengthNeededWithNull - 1};
    }
    do {
        result->resize(valueLengthNeededWithNull - 1);
        if (!std::invoke(std::forward<Fn>(callback), result->data(), result->size() + 1, valueLengthNeededWithNull)) {
            result.reset();
            return result;
        }
    } while (valueLengthNeededWithNull > result->size() + 1);
    if (valueLengthNeededWithNull <= result->size()) { result->resize(valueLengthNeededWithNull - 1); }
    return result;
}

void* getModuleHandle(void* addr) {
    HMODULE hModule = nullptr;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCTSTR>(addr),
        &hModule
    );
    return hModule;
}

std::optional<std::filesystem::path> getModulePath(void* handle, void* process) {
    return adaptFixedSizeToAllocatedResult(
        [module = (HMODULE)handle,
         process](wchar_t* value, size_t valueLength, size_t& valueLengthNeededWithNul) -> bool {
            DWORD  copiedCount{};
            size_t valueUsedWithNul{};
            bool   copyFailed{};
            bool   copySucceededWithNoTruncation{};
            if (process != nullptr) {
                copiedCount      = ::GetModuleFileNameExW(process, module, value, static_cast<DWORD>(valueLength));
                valueUsedWithNul = static_cast<size_t>(copiedCount) + 1;
                copyFailed       = (0 == copiedCount);
                copySucceededWithNoTruncation = !copyFailed && (copiedCount < valueLength - 1);
            } else {
                copiedCount                   = ::GetModuleFileNameW(module, value, static_cast<DWORD>(valueLength));
                valueUsedWithNul              = static_cast<size_t>(copiedCount) + 1;
                copyFailed                    = (0 == copiedCount);
                copySucceededWithNoTruncation = !copyFailed && (copiedCount < valueLength);
            }
            if (copyFailed) { return false; }
            // When the copy truncated, request another try with more space.
            valueLengthNeededWithNul = copySucceededWithNoTruncation ? valueUsedWithNul : (valueLength * 2);
            return true;
        }
    );
}
std::string getModuleFileName(void* handle, void* process) {
    if (auto res = getModulePath(handle, process)) { return u8str2str(res->stem().u8string()); }
    return "unknown module";
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

std::filesystem::path getSystemRoot() {
    auto        len = GetSystemDirectoryA(nullptr, 0);
    std::string buffer(len - 1, '\0');
    GetSystemDirectoryA(buffer.data(), len);
    std::transform(buffer.begin(), buffer.end(), buffer.begin(), ::tolower);
    return buffer;
}
bool isStdoutSupportAnsi() {
    DWORD mode;
    if (GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode)) { return mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING; }
    return false;
}
} // namespace pl::utils
