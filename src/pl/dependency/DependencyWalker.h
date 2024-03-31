#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "pl/internal/Macro.h"

namespace pl::dependency_walker {

struct DependencyIssueItem {
    bool                                                                  mContainsError = false;
    std::filesystem::path                                                 mPath{};
    std::unordered_set<std::string>                                       mMissingModule{};
    std::unordered_map<std::string, std::unordered_set<std::string>>      mMissingProcedure{};
    std::unordered_map<std::string, std::unique_ptr<DependencyIssueItem>> mDependencies{};
};

/**
 * @brief Diagnose the dependency of a library.
 *
 * @param path [in] The path of the library to diagnose.
 * @return result The result of the diagnosis.
 */
PLAPI std::unique_ptr<DependencyIssueItem, void (*)(DependencyIssueItem*)>
      pl_diagnostic_dependency_new(std::filesystem::path const& path);

PLAPI std::unique_ptr<DependencyIssueItem> pl_diagnostic_dependency(std::filesystem::path const& path);

/**
 * @brief Diagnose the dependency of a library.
 *
 * @param path [in] The path of the library to diagnose.
 * @return result The result of the diagnosis.
 *
 * @note the function is an internal function that creates a string instead of a struct.
 */
std::string pl_diagnostic_dependency_string(std::filesystem::path const& path);

} // namespace pl::dependency_walker
