#include "pl/dependency/DependencyWalker.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "demangler/Demangle.h"
#include "fmt/core.h"

#include "pl/dependency/IProvider.h"
#include "pl/dependency/LibrarySearcher.h"
#include "pl/internal/StringUtils.h"
#include "pl/internal/WindowsUtils.h"

namespace fs = std::filesystem;
using namespace pl::dependency_walker;

static std::unique_ptr<IProvider> createProvider(
    std::shared_ptr<LibrarySearcher> const& libSearcher,
    std::u8string const&                    systemRoot,
    std::string const&                      libName,
    fs::path const&                         libPath
) {
    if (libName == "bedrock_server.dll" || libName == "bedrock_server_mod.exe") {
        return std::make_unique<InternalSymbolProvider>();
    } else {
        return std::make_unique<PortableExecutableProvider>(libSearcher, systemRoot, libPath);
    }
}

static std::unique_ptr<DependencyIssueItem> createDiagnosticMessage( // NOLINT(misc-no-recursion)
    fs::path const&                         path,
    std::unique_ptr<IProvider>              provider,
    std::shared_ptr<LibrarySearcher> const& libSearcher,
    std::u8string const&                    systemRoot
) {
    auto result   = std::make_unique<DependencyIssueItem>();
    result->mPath = path;

    std::u8string u8Path = path.u8string();
    // transform to lower case since windows is case-insensitive
    std::transform(u8Path.begin(), u8Path.end(), u8Path.begin(), ::tolower);

    // skip system libraries to reduce unnecessary query, since they are already loaded successfully
    if (u8Path.starts_with(systemRoot)) return nullptr;

    auto imports = provider->getImports();
    for (const auto& [importLib, importFun] : imports) {
        auto importPath    = libSearcher->getLibraryPath(importLib);
        auto containsError = false;
        if (importPath.has_value()) {
            auto subProvider = createProvider(libSearcher, systemRoot, importLib, importPath.value());
            for (const auto& func : importFun) {
                if (!subProvider->queryExport(func)) {
                    result->mMissingProcedure[importLib].insert(func);
                    containsError = true;
                }
            }
            auto sub = createDiagnosticMessage(importPath.value(), std::move(subProvider), libSearcher, systemRoot);
            // add dependency when it contains error, or it's sub-dependency contains error
            if (sub != nullptr || containsError) { result->mDependencies[importLib] = std::move(sub); }

        } else {
            containsError = true;
            result->mMissingModule.insert(importLib);
        }
        if (containsError) result->mContainsError = true;
    }
    return std::move(result);
}

static void printDependencyError( // NOLINT(misc-no-recursion)
    DependencyIssueItem const& item,
    std::ostream&              stream,
    size_t                     depth = 0
) {
    std::string indent(depth * 3 + 3, ' ');
    if (item.mContainsError) {
        stream << indent << fmt::format("module: {}", pl::utils::u8str2str(item.mPath.u8string())) << std::endl;
        if (!item.mMissingModule.empty()) {
            stream << indent << "missing module:" << std::endl;
            for (const auto& missingModule : item.mMissingModule) {
                stream << indent << "|- " << missingModule << std::endl;
            }
        }
        if (!item.mMissingProcedure.empty()) {
            stream << indent << "missing procedure:" << std::endl;
            for (const auto& [module, missingProcedure] : item.mMissingProcedure) {
                stream << indent << "|- " << module << std::endl;
                for (const auto& procedure : missingProcedure) {
                    stream << indent << "|---- " << procedure << std::endl;

                    auto de = demangler::demangle(procedure);
                    if (de != procedure) stream << indent << "|     " << de << std::endl;
                }
            }
        }
        if (!item.mDependencies.empty()) {
            for (const auto& [module, subItem] : item.mDependencies) {
                printDependencyError(*subItem, stream, depth + 1);
            }
        }
    }
}

static void deleteDependencyIssueItem(DependencyIssueItem* item) { delete item; }

std::unique_ptr<DependencyIssueItem, void (*)(DependencyIssueItem*)>
pl::dependency_walker::pl_diagnostic_dependency_new(fs::path const& path) {
    auto libSearcher = LibrarySearcher::getInstance();
    auto systemRoot  = pl::utils::getSystemRoot().u8string();
    auto provider    = std::make_unique<PortableExecutableProvider>(libSearcher, systemRoot, path);
    return {
        createDiagnosticMessage(path, std::move(provider), libSearcher, systemRoot).release(),
        deleteDependencyIssueItem
    };
}

std::unique_ptr<DependencyIssueItem> pl::dependency_walker::pl_diagnostic_dependency(std::filesystem::path const& path
) {
    return std::unique_ptr<DependencyIssueItem>(pl_diagnostic_dependency_new(path).release());
}

std::string pl::dependency_walker::pl_diagnostic_dependency_string(fs::path const& path) {
    auto              result = pl_diagnostic_dependency_new(path);
    std::stringstream stream;
    printDependencyError(*result, stream);
    return stream.str();
}
