#include "pl/dependency/IProvider.h"
#include "pl/dependency/LibrarySearcher.h"
#include "pl/internal/WindowsUtils.h"
#include <fstream>
#include <pe_bliss/pe_bliss.h>
namespace pl::dependency_walker {

PortableExecutableProvider::PortableExecutableProvider(const std::string& libPath) {
    using namespace pe_bliss;
    try {
        std::ifstream peFile(libPath, std::ios::in | std::ios::binary);
        pe_base       image(pe_factory::create_pe(peFile));
        if (image.has_imports()) {
            const imported_functions_list imports = get_imported_functions(image);
            for (const auto& iLib : imports) {
                auto& iLibName = iLib.get_name();
                auto  iLibPath = LibrarySearcher::getInstance()->getLibraryPath(iLibName);
                // skip os libraries to reduce unnecessary query, since they are already loaded successfully
                if (iLibPath.has_value() && iLibPath->starts_with(utils::getSystemRoot())) {
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
