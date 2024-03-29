#include "pl/dependency/IProvider.h"

#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <string>

#include "pe_bliss/pe_base.h"
#include "pe_bliss/pe_exception.h"
#include "pe_bliss/pe_exports.h"
#include "pe_bliss/pe_factory.h"
#include "pe_bliss/pe_imports.h"

#include "pl/dependency/LibrarySearcher.h"

namespace pl::dependency_walker {

PortableExecutableProvider::PortableExecutableProvider(
    const std::shared_ptr<LibrarySearcher>& libSearcher,
    const std::u8string&                    systemRoot,
    const std::filesystem::path&            libPath
) {
    using namespace pe_bliss;
    try {
        std::ifstream peFile(libPath, std::ios::in | std::ios::binary);
        pe_base       image(pe_factory::create_pe(peFile));
        if (image.has_imports()) {
            const imported_functions_list imports = get_imported_functions(image);
            for (const auto& iLib : imports) {
                auto& iLibName = iLib.get_name();
                auto  iLibPath = libSearcher->getLibraryPath(iLibName);
                // skip os libraries to reduce unnecessary query, since they are already loaded successfully
                if (iLibPath.has_value() && iLibPath->u8string().starts_with(systemRoot)) {
                    mImports[iLibName] = {};
                    continue;
                }

                const import_library::imported_list& functions = iLib.get_imported_functions();
                for (const auto& func : functions) {
                    if (func.has_name()) mImports[iLibName].insert(func.get_name());
                }
            }
        }

        if (image.has_exports()) {
            export_info                   info;
            const exported_functions_list exports = get_exported_functions(image, info);

            for (const auto& func : exports) {
                if (func.has_name()) mExports.insert(func.get_name());
            }
        }

    } catch (const pe_exception& e) {
        // if anything goes wrong, just ignore it
    }
}

bool PortableExecutableProvider::queryExport(const std::string& symbol) {
    if (mExports.find(symbol) != mExports.end()) { return true; }
    return false;
}

const IProvider::ImportContainer& PortableExecutableProvider::getImports() { return mImports; }

} // namespace pl::dependency_walker
