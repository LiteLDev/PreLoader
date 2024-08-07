﻿#include "pl/PreLoader.h"

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <string_view>

#include "pl/Config.h"
#include "pl/dependency/DependencyWalker.h"
#include "pl/internal/Logger.h"
#include "pl/internal/StringUtils.h"

#include "nlohmann/json.hpp"

#include <windows.h>

#include <consoleapi.h>
#include <consoleapi2.h>
#include <errhandlingapi.h>
#include <libloaderapi.h>
#include <minwindef.h>
#include <processenv.h>
#include <winbase.h>
#include <winnls.h>
#include <winnt.h>

namespace pl {
namespace fs = std::filesystem;

bool loadLibrary(std::string const& libName, bool showFailInfo = true) {
    if (LoadLibraryW(pl::utils::str2wstr(libName).c_str())) {
        Info("{} Loaded.", pl::utils::u8str2str(std::filesystem::path(libName).stem().u8string()));
        return true;
    }
    if (showFailInfo) {
        DWORD error_message_id = GetLastError();
        Error("Can't load {}", libName);
        Error("Error code: {}", error_message_id);
        LPWSTR message_buffer = nullptr;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr,
            error_message_id,
            NULL,
            (LPWSTR)&message_buffer,
            NULL,
            nullptr
        );
        auto message = pl::utils::wstr2str(message_buffer);
        message.erase(message.find_last_not_of(" \n\r\t") + 1);
        Error("{}", message);
        if (error_message_id == 126 || error_message_id == 127) {
            Error("Analyzing dependency, please wait for a while...");
            Error("Dependency diagnostic:\n{}", dependency_walker::pl_diagnostic_dependency_string(libName));
        }
        LocalFree(message_buffer);
    }
    return false;
}
void loadPreloadNativeMods() {
    namespace fs     = std::filesystem;
    fs::path modsDir = (char8_t const*)(pl_mods_path);
    try {
        for (const auto& entry : fs::directory_iterator(modsDir)) {
            if (!entry.is_directory()) { continue; }
            fs::path manifestPath = entry.path() / "manifest.json";
            if (!fs::exists(manifestPath)) { continue; }
            std::ifstream manifestFile(manifestPath);
            if (manifestFile) {
                try {
                    nlohmann::json manifestJson;
                    manifestFile >> manifestJson;
                    std::string type = manifestJson["type"];
                    if (type == pl_mod_manager_name) {
                        std::string modName  = manifestJson["name"];
                        std::string modEntry = manifestJson["entry"];
                        Info("Preloading: {} <{}>", modName, modEntry);
                        modEntry = (entry.path() / modEntry).string();
                        if (!fs::exists(modEntry)) {
                            Error("Entry not exists: {}", modEntry);
                            continue;
                        }
                        loadLibrary(modEntry);
                    }
                } catch (const nlohmann::json::parse_error& e) { Error("Error parsing manifest file: {}", e.what()); }
            }
        }
    } catch (...) {}
}
void loadConfig();

void init() {
    loadConfig();
    pl::symbol_provider::init();
    loadPreloadNativeMods();
}
} // namespace pl

void openConsole() {
    AllocConsole();
    SetConsoleTitleA("Debug Console");
    HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    INT    hCrt = _open_osfhandle((intptr_t)hCon, _O_TEXT);
    FILE*  hf   = _fdopen(hCrt, "w");
    setvbuf(hf, NULL, _IONBF, 0);
    *stdout = *hf;
    freopen("CONOUT$", "w+t", stdout);
}

[[maybe_unused]] BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call != DLL_PROCESS_ATTACH) return TRUE;

    auto mainExe = pl::utils::getModulePath(nullptr).value();
#ifdef PL_DEBUG
    if (mainExe.filename() == L"Minecraft.Windows.exe") openConsole();
#endif
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    DWORD mode;
    GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // Change current working dir to current module path to make sure we can load mods correctly
    SetCurrentDirectoryW(mainExe.parent_path().c_str());

    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX | SEM_NOALIGNMENTFAULTEXCEPT);
    pl::init();
    return TRUE;
}
