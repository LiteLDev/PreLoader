#pragma once

#include <optional>
#include <string>

namespace pl::fake_symbol {

// generate fakeSymbol for virtual functions
std::optional<std::string> getFakeSymbol(const std::string& fn);

} // namespace pl::fake_symbol
