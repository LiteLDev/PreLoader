//
// Created by rimuruchan on 2023/2/11.
//
#include "pl/Hook.h"

#include <mutex>
#include <unordered_map>

#include <Windows.h>

#include <detours.h>

namespace pl::hook {

struct HookElement {
    FuncPtr detour{};
    FuncPtr* originalFunc{};
    int priority{};
    int id{};

    bool operator<(const HookElement& other) const {
        if (priority != other.priority)
            return priority < other.priority;
        return id < other.id;
    }
};

struct HookData {
    FuncPtr target{};
    FuncPtr origin{};
    FuncPtr start{};
    FuncPtr thunk{};
    int hookId{};
    std::set<HookElement> hooks{};

    inline ~HookData() {
        if (this->thunk != nullptr) {
            VirtualFree(this->thunk, 0, MEM_RELEASE);
        }
    }

    inline void updateCallList() {
        FuncPtr* last = nullptr;
        for (auto& item : this->hooks) {
            if (last == nullptr) {
                this->start = item.detour;
                last = item.originalFunc;
            } else {
                *last = item.detour;
                last = item.originalFunc;
            }
        }
        if (last == nullptr)
            this->start = this->origin;
        else
            *last = this->origin;
    }

    inline int incrementHookId() { return ++hookId; }
};

std::unordered_map<FuncPtr, std::shared_ptr<HookData>> hooks{};
std::mutex hooksMutex{};

struct AutoUnlock {
    ~AutoUnlock() { hooksMutex.unlock(); }
};

FuncPtr createThunk(FuncPtr* target) {
    constexpr auto THUNK_SIZE = 18;
    unsigned char thunkData[THUNK_SIZE] = {0};
    // generate a thunk:
    // mov rax hooker1
    thunkData[0] = 0x48;
    thunkData[1] = 0xB8;
    memcpy(thunkData + 2, &target, sizeof(FuncPtr*));
    // mov rax [rax]
    thunkData[10] = 0x48;
    thunkData[11] = 0x8B;
    thunkData[12] = 0x00;
    // jmp rax
    thunkData[13] = 0xFF;
    thunkData[14] = 0xE0;

    auto thunk = VirtualAlloc(nullptr, THUNK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    memcpy(thunk, thunkData, THUNK_SIZE);
    DWORD dummy;
    VirtualProtect(thunk, THUNK_SIZE, PAGE_EXECUTE_READ, &dummy);
    return thunk;
}

int processHook(FuncPtr target, FuncPtr detour, FuncPtr* originalFunc) {
    FuncPtr tmp = target;
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    int rv = DetourAttach(&tmp, detour);
    DetourTransactionCommit();
    *originalFunc = tmp;
    return rv;
}

[[maybe_unused]] int pl_hook(FuncPtr target, FuncPtr detour, FuncPtr* originalFunc, Priority priority) {
    hooksMutex.lock();
    AutoUnlock unlock;
    auto it = hooks.find(target);
    if (it != hooks.end()) {
        auto hookData = it->second;
        hookData->hooks.insert({detour, originalFunc, priority, hookData->incrementHookId()});
        hookData->updateCallList();
        return ERROR_SUCCESS;
    }

    auto hookData = new HookData{target, target, detour, nullptr, {}, {}};
    hookData->thunk = createThunk(&hookData->start);
    hookData->hooks.insert({detour, originalFunc, priority, hookData->incrementHookId()});
    auto ret = processHook(target, hookData->thunk, &hookData->origin);
    if (ret) {
        delete hookData;
        return ret;
    }
    hookData->updateCallList();
    hooks.emplace(target, std::shared_ptr<HookData>(hookData));
    return ERROR_SUCCESS;
}

[[maybe_unused]] bool pl_unhook(FuncPtr target, FuncPtr detour) {
    hooksMutex.lock();
    AutoUnlock unlock;
    auto hookDataIter = hooks.find(target);
    if (hookDataIter == hooks.end()) {
        return false;
    }
    auto& hookData = hookDataIter->second;
    for (auto it = hookData->hooks.begin(); it != hookData->hooks.end(); ++it) {
        if (it->detour != detour)
            continue;
        hookData->hooks.erase(it);
        hookData->updateCallList();
        return true;
    }
    return false;
}

} // namespace pl::hook
