#pragma once

#include "pl/Hook.h"           // IWYU pragma: export
#include "pl/SymbolProvider.h" // IWYU pragma: export
#include "pl/internal/Macro.h" // IWYU pragma: export

#include <string_view>
namespace pl::plugin {

constexpr std::string_view NativePluginManagerName = "preload-native";

}