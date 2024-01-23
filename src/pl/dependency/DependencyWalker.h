#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "pl/internal/Macro.h"

namespace pl::dependency_walker {

struct DependencyIssueItem{
    bool mContainsError = false;
    std::string mPath{};
    std::unordered_set<std::string> mMissingModule{};
    std::unordered_map<std::string,std::unordered_set<std::string>> mMissingProcedure{};
    std::unordered_map<std::string,std::unique_ptr<DependencyIssueItem>> mDependencies{};
};

/**
 * @brief Diagnose the dependency of a library.
 *
 * @param path [in] The path of the library to diagnose.
 * @return result The result of the diagnosis.
 */
PLAPI std::unique_ptr<DependencyIssueItem> pl_diagnostic_dependency(std::string path);

/**
 * @brief Diagnose the dependency of a library.
 *
 * @param path [in] The path of the library to diagnose.
 * @return result The result of the diagnosis.
 *
 * @note the function is an internal function that creates a string instead of a struct.
 */
std::string pl_diagnostic_dependency_string(std::string path);

}