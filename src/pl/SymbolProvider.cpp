#include "pl/SymbolProvider.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <list>
#include <string>
#include <string_view>

#include "parallel_hashmap/btree.h"
#include "parallel_hashmap/phmap.h"

#include "PDB.h"
#include "PDB_CoalescedMSFStream.h"
#include "PDB_DBIStream.h"
#include "PDB_DBITypes.h"
#include "PDB_ImageSectionStream.h"
#include "PDB_InfoStream.h"
#include "PDB_ModuleInfoStream.h"
#include "PDB_ModuleSymbolStream.h"
#include "PDB_PublicSymbolStream.h"
#include "PDB_RawFile.h"
#include "PDB_Types.h"

#include "snappy.h"

#include "pl/internal/FakeSymbol.h"
#include "pl/internal/Logger.h"
#include "pl/internal/MemoryFile.h"
#include "pl/internal/PdbUtils.h"
#include "pl/internal/WindowsUtils.h"

#include <libloaderapi.h>

using PDB::ArrayView;

using namespace pl::utils;

// base address of bedrock_server.exe
static uintptr_t imageBaseAddr;

// phmap hash maps is thread-safe when reading
using Symbol2RvaMap    = phmap::flat_hash_map<std::string, uint32_t>;
using Rva2SymbolMap    = phmap::btree_map<uint32_t, std::vector<std::string const*>>;
Symbol2RvaMap* funcMap = nullptr;
Rva2SymbolMap* rvaMap  = nullptr;

void initFunctionMap(PDB::RawFile const& rawPdbFile, PDB::DBIStream const& dbiStream) {
    Info("Loading symbols from pdb...");

    PDB::ImageSectionStream const imageSectionStream = dbiStream.CreateImageSectionStream(rawPdbFile);
    PDB::CoalescedMSFStream const symbolRecordStream = dbiStream.CreateSymbolRecordStream(rawPdbFile);
    PDB::PublicSymbolStream const publicSymbolStream = dbiStream.CreatePublicSymbolStream(rawPdbFile);
    PDB::ModuleInfoStream const   moduleInfoStream   = dbiStream.CreateModuleInfoStream(rawPdbFile);

    ArrayView<PDB::ModuleInfoStream::Module> const modules            = moduleInfoStream.GetModules();
    ArrayView<PDB::HashRecord> const               publicSymbolRecord = publicSymbolStream.GetRecords();

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

void initFunctionMap(std::string_view compressed) {
    Info("Loading symbols from data...");
    funcMap = new Symbol2RvaMap{};
    std::string data;
    snappy::Uncompress(compressed.data(), compressed.size(), &data);
    uint32_t rva{0};
    for (size_t i = 0; i < data.size(); i++) {
        uint8_t    c          = data[i++];
        bool const isFunction = c & (1 << 0);
        bool const isPublic   = c & (1 << 1);
        bool const skipped    = c & (1 << 2);
        uint32_t   rrva{0};
        int        shift_amount = 0;
        do {
            rrva         |= (uint32_t)(data[i] & 0x7F) << shift_amount;
            shift_amount += 7;
        } while ((data[i++] & 0x80) != 0);
        rva        += rrva;
        auto begin  = i;
        i           = data.find_first_of('\n', begin);
        if (i == std::string::npos) { break; }
        if (skipped) { continue; }
        std::string name(data, begin, i - begin);
        if (isPublic) {
            auto fake = pl::fake_symbol::getFakeSymbol(name);
            if (fake.has_value()) funcMap->emplace(fake.value(), rva);
            if (isFunction) {
                // MCVAPI
                fake = pl::fake_symbol::getFakeSymbol(name, true);
                if (fake.has_value()) funcMap->emplace(fake.value(), rva);
            }
        }
        funcMap->emplace(std::move(name), rva);
    }
    Info("Loaded {} symbols", funcMap->size());
    fflush(stdout);
}
void initReverseLookup() {
    Info("Loading Reverse Lookup Table");
    rvaMap = new Rva2SymbolMap();
    for (auto& [sym, rva] : *funcMap) { (*rvaMap)[rva].push_back(&sym); }
}
namespace pl::symbol_provider {

void init() {
    const wchar_t* const pdbPath = LR"(./bedrock_server.pdb)";
    MemoryFile           pdbFile = MemoryFile::Open(pdbPath);
    if (!pdbFile.baseAddress && !std::filesystem::exists(LR"(./bedrock_symbol_data)")) {
        Error("bedrock_server.pdb not found");
        return;
    } else if (!pdbFile.baseAddress) {
        std::ifstream fRead;
        fRead.open(LR"(./bedrock_symbol_data)", std::ios_base::in | std::ios_base::binary);
        if (!fRead.is_open()) {
            Error("can't open bedrock_symbol_data");
            return;
        }
        std::string data((std::istreambuf_iterator<char>(fRead)), {});
        fRead.close();
        initFunctionMap(data);
    } else {
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
    }
    initReverseLookup();
    imageBaseAddr = (uintptr_t)GetModuleHandle(nullptr);
}

void* pl_resolve_symbol(const char* symbolName) {
    if (!funcMap) {
        Error("pl_resolve_symbol called before init");
        return nullptr;
    }
    auto iter = funcMap->find(std::string_view(symbolName));
    if (iter == funcMap->end()) {
        Error("Could not find function in memory: {}", symbolName);
        Error("Module: {}", pl::utils::getCallerModuleFileName());
        return nullptr;
    }
    return (void*)(imageBaseAddr + iter->second);
}

void* pl_resolve_symbol_silent(const char* symbolName) {
    if (!funcMap) { return nullptr; }
    auto iter = funcMap->find(std::string_view(symbolName));
    if (iter == funcMap->end()) { return nullptr; }
    return (void*)(imageBaseAddr + iter->second);
}

void* pl_resolve_symbol_silent_n(const char* symbolName, size_t size) {
    if (!funcMap) { return nullptr; }
    auto iter = funcMap->find(std::string_view(symbolName, size));
    if (iter == funcMap->end()) { return nullptr; }
    return (void*)(imageBaseAddr + iter->second);
}

template <typename C>
typename C::const_iterator find_key_less_equal(const C& c, const typename C::key_type& key) {
    auto iter = c.upper_bound(key);
    if (iter == c.cbegin()) return c.cend();
    return --iter;
}

const char* const* pl_lookup_symbol_disp(void* func, size_t* resultLength, unsigned int* displacement) {
    if (!rvaMap) {
        if (resultLength) *resultLength = 0;
        Error("pl_lookup_symbol called before init");
        return nullptr;
    }
    auto rva        = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(func) - imageBaseAddr);
    auto lastSymbol = find_key_less_equal(*rvaMap, rva);
    if (lastSymbol == rvaMap->end()) {
        if (resultLength) *resultLength = 0;
        return nullptr;
    }
    if (displacement) *displacement = rva - lastSymbol->first;
    auto size    = lastSymbol->second.size();
    auto result  = new char*[size + 1];
    result[size] = nullptr; // nullptr terminated
    if (resultLength) *resultLength = size;
    size_t i = 0;
    for (auto& sym : lastSymbol->second) {
        result[i] = new char[sym->length() + 1];
        std::copy(sym->begin(), sym->end(), result[i]);
        result[i][sym->length()] = '\0';
        i++;
    }
    return result;
}
const char* const* pl_lookup_symbol(void* func, size_t* resultLength) {
    return pl_lookup_symbol_disp(func, resultLength, nullptr);
}
void pl_free_lookup_result(const char* const* result) {
    for (int i = 0; result[i] != nullptr; i++) { delete[] result[i]; }
    delete[] result;
}
} // namespace pl::symbol_provider
