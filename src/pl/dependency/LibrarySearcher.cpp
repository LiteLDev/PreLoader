#include "pl/dependency/LibrarySearcher.h"
#include "pl/internal/StringUtils.h"
#include "pl/internal/WindowsUtils.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <pe_bliss/pe_bliss.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

std::shared_ptr<pl::dependency_walker::LibrarySearcher> pl::dependency_walker::LibrarySearcher::getInstance() {
    static size_t                           envPathHash = 0;
    static std::shared_ptr<LibrarySearcher> instance    = nullptr;

    auto envPath        = std::string_view(std::getenv("PATH"));
    auto envPathHashNew = std::hash<std::string_view>()(envPath);

    // create new LibrarySearcher instance only when ENV:PATH changed
    if (envPathHash != envPathHashNew) {
        instance    = std::make_shared<pl::dependency_walker::LibrarySearcher>(LibrarySearcher(envPath));
        envPathHash = envPathHashNew;
    }
    return instance;
}

//-1. Sxs manifests <-- not supported
// 0. Current loaded directory
// 1. C:\Windows\System32
// 2. Working directory
// 3. PATH
pl::dependency_walker::LibrarySearcher::LibrarySearcher(std::string_view envPath) {
    mLibraryPathList.insert(std::filesystem::path(utils::getSystemRoot()));
    mLibraryPathList.insert(std::filesystem::current_path());
    auto pathList = pl::utils::split(envPath, ";");
    for (const auto& path : pathList) { mLibraryPathList.insert(std::filesystem::path(path)); }
}

std::optional<std::filesystem::path> pl::dependency_walker::LibrarySearcher::getLibraryPath(std::string libName) const {
    static auto apiSetSchema = parseApiSetSchema();

    // Windows's filesystem is case-insensitive
    std::transform(libName.begin(), libName.end(), libName.begin(), ::tolower);

    // handle api set schema redirection
    if (apiSetSchema.find(libName) != apiSetSchema.end()) { libName = apiSetSchema.at(libName); }

    // try to get the module from currently loaded module
    auto handler = GetModuleHandleA(libName.c_str());
    if (handler != nullptr) {
        char path[MAX_PATH];
        GetModuleFileNameA(handler, path, MAX_PATH);
        return path;
    }

    for (const auto& p : mLibraryPathList) {
        auto file = p / libName;
        if (exists(file)) { return file; }
    }
    return std::nullopt;
}

struct API_SET_NAMESPACE {
    uint32_t Version;
    uint32_t Size;
    uint32_t Flags;
    uint32_t Count;
    uint32_t EntryOffset;
    uint32_t HashOffset;
    uint32_t HashFactor;
};
struct API_SET_HASH_ENTRY {
    uint32_t Hash;
    uint32_t Index;
};
struct API_SET_NAMESPACE_ENTRY {
    uint32_t Flags;
    uint32_t NameOffset;
    uint32_t NameLength;
    uint32_t HashedLength;
    uint32_t ValueOffset;
    uint32_t ValueCount;
};
struct API_SET_VALUE_ENTRY {
    uint32_t Flags;
    uint32_t NameOffset;
    uint32_t NameLength;
    uint32_t ValueOffset;
    uint32_t ValueLength;
};

std::unordered_map<std::string, std::string> pl::dependency_walker::LibrarySearcher::parseApiSetSchema() {
    using namespace pe_bliss;
    std::unordered_map<std::string, std::string> result;
    try {
        auto          path = std::filesystem::path(utils::getSystemRoot()) / "apisetschema.dll";
        std::string   apiSetSection;
        std::ifstream peFile(path, std::ios::in | std::ios::binary);
        pe_base       image(pe_factory::create_pe(peFile));
        for (const auto& iSec : image.get_image_sections()) {
            if (iSec.get_name() == ".apiset") { apiSetSection = iSec.get_raw_data(); }
        }

        auto* apiSetNamespace      = (API_SET_NAMESPACE*)(apiSetSection.data());
        auto  namespace_addr       = (uintptr_t)apiSetNamespace;
        auto* apiSetNamespaceEntry = (API_SET_NAMESPACE_ENTRY*)(namespace_addr + apiSetNamespace->EntryOffset);

        for (auto i = 0; i < apiSetNamespace->Count; i++) {
            std::wstring originName(
                (wchar_t*)(namespace_addr + apiSetNamespaceEntry->NameOffset),
                apiSetNamespaceEntry->NameLength / 2
            );
            originName.append(L".dll");
            auto* valueEntry = (API_SET_VALUE_ENTRY*)(namespace_addr + apiSetNamespaceEntry->ValueOffset);
            for (auto j = 0; j < apiSetNamespaceEntry->ValueCount; j++) {
                if (j == 0) { // only take the first one
                    std::wstring forwardName(
                        (wchar_t*)(namespace_addr + valueEntry->ValueOffset),
                        valueEntry->ValueLength / 2
                    );
                    result[utils::wstr2str(originName)] = utils::wstr2str(forwardName);
                }
                valueEntry++;
            }
            apiSetNamespaceEntry++;
        }
        return result;
    } catch (...) {
        // unable to parse api set schema
        return {};
    }
}
