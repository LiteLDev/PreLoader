#include <cstdlib>
#include <string>

#include <windows.h>

#include "preloader.h"

#pragma comment(linker, "/export:GetServerSymbol=LLPreLoader.dlsym_real")

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {

        // Changing code page to UTF-8
        // not using SetConsoleOutputCP here, avoid color output broken
        system("chcp 65001 > nul");

        // For #683, Change CWD to current module path
        auto buffer = new wchar_t[MAX_PATH];
        GetModuleFileName(hModule, buffer, MAX_PATH);
        std::wstring path(buffer);
        auto cwd = path.substr(0, path.find_last_of('\\'));
        SetCurrentDirectoryW(cwd.c_str());

        pl::Init();
    }
    if (ul_reason_for_call == DLL_PROCESS_DETACH) {
    }
    return TRUE;
}
