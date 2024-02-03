#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace pl::dependency_walker {

class LibrarySearcher {
private:
    std::unordered_set<std::filesystem::path> mLibraryPathList;
    explicit LibrarySearcher(std::string_view envPath);
    static std::unordered_map<std::string, std::string> parseApiSetSchema();

public:
    static std::shared_ptr<LibrarySearcher> getInstance();
    /// the function search directories and return the path that contains target library
    /// @param libName the library to search, case insensitive
    [[nodiscard]] std::optional<std::filesystem::path> getLibraryPath(std::string libName) const;
};
} // namespace pl::dependency_walker
