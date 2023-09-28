#pragma once

#include <Windows.h>

namespace pl::utils {

struct MemoryFile {
    HANDLE file;
    HANDLE fileMapping;
    void*  baseAddress;

    static MemoryFile Open(const wchar_t* path) {
        HANDLE file =
            CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
        if (file == INVALID_HANDLE_VALUE) { return {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, nullptr}; }

        HANDLE fileMapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (fileMapping == nullptr) {
            CloseHandle(file);

            return {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, nullptr};
        }

        void* baseAddress = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);
        if (baseAddress == nullptr) {
            CloseHandle(fileMapping);
            CloseHandle(file);

            return {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, nullptr};
        }

        return {file, fileMapping, baseAddress};
    }

    static void Close(MemoryFile& handle) {
        UnmapViewOfFile(handle.baseAddress);
        CloseHandle(handle.fileMapping);
        CloseHandle(handle.file);

        handle.file        = nullptr;
        handle.fileMapping = nullptr;
        handle.baseAddress = nullptr;
    }
};
} // namespace pl::utils
