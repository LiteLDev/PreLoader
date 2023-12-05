#include "pl/SymbolProvider.h"

#include <cstdint>
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

#include "pl/internal/ApHash.h"
#include "pl/internal/FakeSymbol.h"
#include "pl/internal/Logger.h"
#include "pl/internal/MemoryFile.h"
#include "pl/internal/PdbUtils.h"
#include "pl/internal/WindowsUtils.h"

using namespace pl::utils;

// base address of bedrock_server.exe
static uintptr_t imageBaseAddr;

// phmap hash maps is thread-safe when reading
using Symbol2RvaMap    = phmap::flat_hash_map<std::string, uint32_t>;
using Rva2SymbolMap    = phmap::flat_hash_map<uint32_t, std::list<const std::string*>>;
Symbol2RvaMap* funcMap = nullptr;
Rva2SymbolMap* rvaMap  = nullptr;

void initFunctionMap(const PDB::RawFile& rawPdbFile, const PDB::DBIStream& dbiStream) {
    Info("Loading symbols from pdb...");

    const PDB::ImageSectionStream imageSectionStream            = dbiStream.CreateImageSectionStream(rawPdbFile);
    const PDB::CoalescedMSFStream symbolRecordStream            = dbiStream.CreateSymbolRecordStream(rawPdbFile);
    const PDB::PublicSymbolStream publicSymbolStream            = dbiStream.CreatePublicSymbolStream(rawPdbFile);
    const PDB::ModuleInfoStream   moduleInfoStream              = dbiStream.CreateModuleInfoStream(rawPdbFile);
    const PDB::ArrayView<PDB::ModuleInfoStream::Module> modules = moduleInfoStream.GetModules();

    const PDB::ArrayView<PDB::HashRecord> publicSymbolRecord = publicSymbolStream.GetRecords();

    // pre alloc hash buckets, 1.2x for public symbols
    funcMap = new Symbol2RvaMap((size_t)((double)publicSymbolRecord.GetLength() * 1.2));

    // public symbols are those symbols that are visible to the linker
    // usually comes with mangling(like ?foo@@YAXXZ)
    for (const PDB::HashRecord& hashRecord : publicSymbolRecord) {
        const PDB::CodeView::DBI::Record* record = publicSymbolStream.GetRecord(symbolRecordStream, hashRecord);
        const uint32_t                    rva =
            imageSectionStream.ConvertSectionOffsetToRVA(record->data.S_PUB32.section, record->data.S_PUB32.offset);
        if (rva == 0u) continue;
        funcMap->emplace(record->data.S_PUB32.name, rva);

        auto fake = pl::fake_symbol::getFakeSymbol(record->data.S_PUB32.name);
        if (fake.has_value()) funcMap->emplace(fake.value(), rva);

        // MCVAPI
        fake = pl::fake_symbol::getFakeSymbol(record->data.S_PUB32.name, true);
        if (fake.has_value()) funcMap->emplace(fake.value(), rva);
    }

    // module symbols are those symbols that are not visible to the linker
    // usually anonymous implementation/cleanup dtor/etc..., without mangling
    for (const PDB::ModuleInfoStream::Module& module : modules) {
        if (!module.HasSymbolStream()) { continue; }

        const PDB::ModuleSymbolStream moduleSymbolStream = module.CreateSymbolStream(rawPdbFile);
        moduleSymbolStream.ForEachSymbol([&imageSectionStream](const PDB::CodeView::DBI::Record* record) {
            if (record->header.kind == PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32) {
                uint32_t rva = imageSectionStream.ConvertSectionOffsetToRVA(
                    record->data.S_LPROC32.section,
                    record->data.S_LPROC32.offset
                );
                if (rva == 0u) return;

                std::string_view name = record->data.S_LPROC32.name;
                bool             skip = true;
                // symbol with '`' prefix is special symbol
                if (name.starts_with("`")) {
                    if (name.starts_with("`anonymous namespace'")) { skip = false; }
                } else {
                    if (name.starts_with("std::_Func_impl_no_alloc") || name.find("`dynamic") != std::string_view::npos)
                        skip = true;
                    else skip = false;
                }
                if (!skip) { funcMap->emplace(record->data.S_LPROC32.name, rva); }
            }
        });
    }


    Info("Loaded {} symbols", funcMap->size());
    fflush(stdout);
}

void initReverseLookup() {
    Info("Loading Reverse Lookup Table");
    rvaMap = new Rva2SymbolMap(funcMap->size());
    for (auto& [sym, rva] : *funcMap) { (*rvaMap)[rva].push_back(&sym); }
}

namespace pl::symbol_provider {

void init() {
    const wchar_t* const pdbPath = LR"(./bedrock_server.pdb)";
    MemoryFile           pdbFile = MemoryFile::Open(pdbPath);
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
    initFunctionMap(rawPdbFile, dbiStream);
    initReverseLookup();
    imageBaseAddr = (uintptr_t)GetModuleHandle(nullptr);
}

void* pl_resolve_symbol(const char* symbolName, bool printNotFound) {
    if (!funcMap) {
        Error("pl_resolve_symbol called before init");
        return nullptr;
    }
    auto iter = funcMap->find(std::string(symbolName));
    if (iter != funcMap->end()) {
        return (void*)(imageBaseAddr + iter->second);
    } else if (printNotFound) {
        Error("Could not find function in memory: {}", symbolName);
        Error("Plugin: {}", pl::utils::GetCallerModuleFileName());
    }
    return nullptr;
}

const char* const* pl_lookup_symbol(void* func, size_t* resultLength) {
    if (!rvaMap) {
        if (resultLength) *resultLength = 0;
        Error("pl_lookup_symbol called before init");
        return nullptr;
    }
    auto funcAddr = reinterpret_cast<uintptr_t>(func);
    auto it       = rvaMap->find(static_cast<uint32_t>(funcAddr - imageBaseAddr));
    if (it == rvaMap->end()) {
        if (resultLength) *resultLength = 0;
        return nullptr;
    }

    auto result = new char*[it->second.size() + 1];
    if (resultLength) *resultLength = it->second.size();

    size_t i = 0;
    for (auto& sym : it->second) {
        result[i] = new char[sym->length() + 1];
        std::copy(sym->begin(), sym->end(), result[i]);
        result[i][sym->length()] = '\0';
    }
    result[it->second.size()] = nullptr; // nullptr terminated
    return result;
}

void pl_free_lookup_result(const char* const* result) {
    for (int i = 0; result[i] != nullptr; i++) { delete[] result[i]; }
    delete[] result;
}

} // namespace pl::symbol_provider
