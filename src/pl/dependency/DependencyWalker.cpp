#include "pl/dependency/DependencyWalker.h"
#include "pl/dependency/IProvider.h"
#include "pl/dependency/LibrarySearcher.h"
#include "pl/internal/StringUtils.h"
#include "pl/internal/WindowsUtils.h"
#include <demangler/Demangle.h>
#include <fmt/format.h>
#include <iostream>
#include <sstream>
#include <string>

using namespace pl::dependency_walker;

static std::unique_ptr<IProvider>
createProvider(std::shared_ptr<LibrarySearcher> libSearcher, const std::string& libName, const std::string& libPath) {
    if (libName == "bedrock_server.dll" || libName == "bedrock_server_mod.exe") {
        return std::make_unique<InternalSymbolProvider>();
    } else {
        return std::make_unique<PortableExecutableProvider>(libSearcher, libPath);
    }
}

static std::unique_ptr<DependencyIssueItem> createDiagnosticMessage(
    std::string                             path,
    std::unique_ptr<IProvider>              provider,
    const std::shared_ptr<LibrarySearcher>& libSearcher,
    const std::string&                      systemRoot
) {
    auto result   = std::make_unique<DependencyIssueItem>();
    result->mPath = path;

    // transform to lower case since windows is case-insensitive
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);

    // skip system libraries to reduce unnecessary query, since they are already loaded successfully
    if (path.starts_with(systemRoot)) return nullptr;

    auto imports = provider->getImports();
    for (const auto& [importLib, importFun] : imports) {
        auto importPath    = libSearcher->getLibraryPath(importLib);
        auto containsError = false;
        if (importPath.has_value()) {
            auto subProvider = createProvider(libSearcher, importLib, importPath.value());
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

static void
printDependencyError(const std::unique_ptr<DependencyIssueItem>& item, std::ostream& stream, size_t depth = 0) {
    std::string indent(depth * 3 + 3, ' ');
    if (item->mContainsError) {
        stream << indent << fmt::format("module: {}", item->mPath) << std::endl;
        if (!item->mMissingModule.empty()) {
            stream << indent << "missing module:" << std::endl;
            for (const auto& missingModule : item->mMissingModule) {
                stream << indent << "|- " << missingModule << std::endl;
            }
        }
        if (!item->mMissingProcedure.empty()) {
            stream << indent << "missing procedure:" << std::endl;
            for (const auto& [module, missingProcedure] : item->mMissingProcedure) {
                stream << indent << "|- " << module << std::endl;
                for (const auto& procedure : missingProcedure) {
                    stream << indent << "|---- " << procedure << std::endl;

                    auto de = demangler::demangle(procedure);
                    if (de != procedure) stream << indent << "|     " << de << std::endl;
                }
            }
        }
        if (!item->mDependencies.empty()) {
            for (const auto& [module, subItem] : item->mDependencies) {
                printDependencyError(subItem, stream, depth + 1);
            }
        }
    }
}

std::unique_ptr<DependencyIssueItem> pl::dependency_walker::pl_diagnostic_dependency(std::string path) {
    auto libSearcher = LibrarySearcher::getInstance();
    auto systemRoot  = pl::utils::getSystemRoot();
    auto provider    = std::make_unique<PortableExecutableProvider>(libSearcher, path);
    return createDiagnosticMessage(path, std::move(provider), libSearcher, systemRoot);
}

std::string pl::dependency_walker::pl_diagnostic_dependency_string(std::string path) {
    auto              result = pl_diagnostic_dependency(path);
    std::stringstream stream;
    printDependencyError(result, stream);
    return stream.str();
}