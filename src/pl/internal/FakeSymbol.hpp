#pragma once

#include <optional>
#include <string>

namespace pl::fake_symbol {

// generate fakeSymbol for virtual functions
std::optional<std::string> getFakeSymbol(const std::string& fn, bool removeVirtual = false);

} // namespace pl::fake_symbol
