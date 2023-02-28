#include "pl/utils/WindowsUtils.h"

#include <filesystem>

#include <Windows.h>

#include <Psapi.h>

std::string pl::utils::GetCallerModuleFileName(unsigned long FramesToSkip) {
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
        wchar_t buf[MAX_PATH] = {0};
        GetModuleFileNameExW(GetCurrentProcess(), hModule, buf, MAX_PATH);
        return std::filesystem::path(buf).filename().u8string();
    }
    return "Unknown";
}
