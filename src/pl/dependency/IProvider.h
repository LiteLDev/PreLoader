#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "pl/dependency/LibrarySearcher.h"

namespace pl::dependency_walker {
// IProvider is a common interface to get import/export symbols
class IProvider {
public:
    using ExportContainer = std::unordered_set<std::string>;
    using ImportContainer = std::unordered_map<std::string, std::unordered_set<std::string>>;
    /// query a function is exported in the module
    virtual bool                   queryExport(const std::string&) = 0;
    virtual const ImportContainer& getImports()                    = 0;
    virtual ~IProvider()                                           = default;
};

/// PortableExecutableProvider provides access to export and import table for portable executable
class PortableExecutableProvider : public IProvider {
private:
    ExportContainer mExports;
    ImportContainer mImports;

public:
    explicit PortableExecutableProvider(
        const std::shared_ptr<LibrarySearcher>& libSearcher,
        const std::u8string&                    systemRoot,
        const std::filesystem::path&            path
    );
    bool                   queryExport(const std::string&) override;
    const ImportContainer& getImports() override;
};

/// InternalSymbolProvider provide a proxy to bds apis query in SymbolProvider, since bds is loaded successfully now,
/// imports will be ignore
class InternalSymbolProvider : public IProvider {
public:
    InternalSymbolProvider();
    bool                   queryExport(const std::string&) override;
    const ImportContainer& getImports() override;
};

}; // namespace pl::dependency_walker
