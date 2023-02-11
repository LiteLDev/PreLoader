#include <cstdio>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <PDB.h>
#include <PDB_DBIStream.h>
#include <PDB_InfoStream.h>
#include <PDB_RawFile.h>

#include <parallel_hashmap/phmap.h>

#include "pl/utils/ApHash.h"
#include "pl/utils/FakeSymbol.hpp"
#include "pl/utils/Logger.h"
#include "pl/utils/MemoryFile.h"
#include "pl/utils/PdbUtils.h"
#include "pl/utils/WindowsUtils.h"

#include <Windows.h>

using std::string, std::string_view;
using std::unordered_map, std::unordered_multimap, std::vector;

using namespace pl::utils;

bool fastDlsymState = false;
bool initialized = false;
static uintptr_t imageBaseAddr;

std::mutex dlsymLock{};
phmap::flat_hash_map<string, int, ap_hash>* funcMap;
unordered_multimap<int, string*>* rvaMap;

void testFuncMap() {
    return;
    // TODO: Update check symbol
    constexpr auto TEST_SYMBOL = "?initializeLogging@DedicatedServer@@AEAAXXZ";
    auto handle = GetModuleHandle(nullptr);
    auto exportedFuncAddr = GetProcAddress(handle, TEST_SYMBOL);
    void* symDbFn = nullptr;
    auto iter = funcMap->find(string(TEST_SYMBOL));
    if (iter != funcMap->end()) {
        symDbFn = (void*)(imageBaseAddr + iter->second);
    }
    if (symDbFn != exportedFuncAddr) {
        Error("Could not find critical symbol in pdb");
        fastDlsymState = false;
    }
}

void initFastDlsym(const PDB::RawFile& rawPdbFile, const PDB::DBIStream& dbiStream) {
    Info("Loading symbols from pdb...");
    funcMap = new phmap::flat_hash_map<string, int, ap_hash>;
    const PDB::ImageSectionStream imageSectionStream = dbiStream.CreateImageSectionStream(rawPdbFile);
    const PDB::CoalescedMSFStream symbolRecordStream = dbiStream.CreateSymbolRecordStream(rawPdbFile);
    const PDB::PublicSymbolStream publicSymbolStream = dbiStream.CreatePublicSymbolStream(rawPdbFile);

    const PDB::ArrayView<PDB::HashRecord> hashRecords = publicSymbolStream.GetRecords();

    for (const PDB::HashRecord& hashRecord : hashRecords) {
        const PDB::CodeView::DBI::Record* record = publicSymbolStream.GetRecord(symbolRecordStream, hashRecord);
        const uint32_t rva =
            imageSectionStream.ConvertSectionOffsetToRVA(record->data.S_PUB32.section, record->data.S_PUB32.offset);
        if (rva == 0u)
            continue;
        funcMap->emplace(record->data.S_PUB32.name, rva);

        auto fake = pl::fake_symbol::getFakeSymbol(record->data.S_PUB32.name);
        if (fake.has_value())
            funcMap->emplace(fake.value(), rva);
    }

    const PDB::ModuleInfoStream moduleInfoStream = dbiStream.CreateModuleInfoStream(rawPdbFile);
    const PDB::ArrayView<PDB::ModuleInfoStream::Module> modules = moduleInfoStream.GetModules();

    for (const PDB::ModuleInfoStream::Module& module : modules) {
        if (!module.HasSymbolStream())
            continue;
        const PDB::ModuleSymbolStream moduleSymbolStream = module.CreateSymbolStream(rawPdbFile);
        moduleSymbolStream.ForEachSymbol([&imageSectionStream](const PDB::CodeView::DBI::Record* record) {
            if (record->header.kind != PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32)
                return;
            const uint32_t rva = imageSectionStream.ConvertSectionOffsetToRVA(record->data.S_LPROC32.section,
                                                                              record->data.S_LPROC32.offset);
            string name = record->data.S_LPROC32.name;
            if (name.find("lambda") != std::string::npos) {
                funcMap->emplace(record->data.S_LPROC32.name, rva);
            }
        });
    }
    fastDlsymState = true;
    testFuncMap();
    Info("Fast Dlsym Loaded <{}>", funcMap->size());
    fflush(stdout);
}

void initReverseLookup() {
    Info("Loading Reverse Lookup Table");
    rvaMap = new unordered_multimap<int, string*>(funcMap->size());
    for (auto& pair : *funcMap) {
        rvaMap->insert({pair.second, (string*)&pair.first});
    }
}

#include "pl/SymbolProvider.h"

namespace pl::symbol_provider {

void init() {
    if (initialized)
        return;
    const wchar_t* const pdbPath = LR"(./bedrock_server.pdb)";
    MemoryFile pdbFile = MemoryFile::Open(pdbPath);
    if (!pdbFile.baseAddress) {
        Error("bedrock_server.pdb not found");
        return;
    }
    if (handleError(PDB::ValidateFile(pdbFile.baseAddress))) {
        MemoryFile::Close(pdbFile);
        return;
    }
    const PDB::RawFile rawPdbFile = PDB::CreateRawFile(pdbFile.baseAddress);
    if (handleError(PDB::HasValidDBIStream(rawPdbFile))) {
        MemoryFile::Close(pdbFile);
        return;
    }
    const PDB::InfoStream infoStream(rawPdbFile);
    if (infoStream.UsesDebugFastLink()) {
        MemoryFile::Close(pdbFile);
        return;
    }
    const PDB::DBIStream dbiStream = PDB::CreateDBIStream(rawPdbFile);
    if (!checkValidDBIStreams(rawPdbFile, dbiStream)) {
        MemoryFile::Close(pdbFile);
        return;
    }
    initFastDlsym(rawPdbFile, dbiStream);
    initReverseLookup();
    imageBaseAddr = (uintptr_t)GetModuleHandle(nullptr);
    initialized = true;
}

[[maybe_unused]] void* pl_resolve_symbol(const char* symbolName) {
    static_assert(sizeof(HMODULE) == 8);
    dlsymLock.lock();
    if (!fastDlsymState)
        return nullptr;
    auto iter = funcMap->find(string(symbolName));
    if (iter != funcMap->end()) {
        dlsymLock.unlock();
        return (void*)(imageBaseAddr + iter->second);
    } else {
        Error("Could not find function in memory: {}", symbolName);
        Error("Plugin: {}", pl::utils::GetCallerModuleFileName());
    }
    dlsymLock.unlock();
    return nullptr;
}

[[maybe_unused]] void pl_lookup_symbol(void* func, size_t* resultLenght, const char*** result) {
    if (!rvaMap) {
        *resultLenght = 0;
        *result = nullptr;
        return;
    }
    auto funcAddr = reinterpret_cast<uintptr_t>(func);
    int rva = static_cast<int>(funcAddr - imageBaseAddr);
    auto const iter = rvaMap->equal_range(rva);
    std::vector<string> vec;
    for (auto it = iter.first; it != iter.second; ++it) {
        vec.push_back(*it->second);
    }
    size_t length = vec.size();
    *resultLenght = length;
    *result = new const char*[length + 1];
    for (int i = 0; i < vec.size(); i++) {
        size_t strSize = vec[i].size() + 1;
        const char* str = new char[strSize];
        memcpy((void*)str, vec[i].c_str(), strSize);
        (*result)[i] = str;
    }
    (*result)[vec.size()] = nullptr;
}

[[maybe_unused]] void pl_free_lookup_result(const char** result) {
    for (int i = 0; result[i] != nullptr; i++) {
        delete[] result[i];
    }
    delete[] result;
}

} // namespace pl::symbol_provider

// TODO: for compability, wait for delete
PLCAPI void* dlsym_real(const char* symbol) { return pl::symbol_provider::pl_resolve_symbol(symbol); }
