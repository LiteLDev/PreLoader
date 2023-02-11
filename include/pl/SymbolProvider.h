#pragma once

#include "pl/utils/Macro.h"

namespace pl::symbol_provider {

void init();

PLCAPI void* pl_resolve_symbol(const char* symbolName);

/**
 * @brief Get the symbol name of a function address.
 *
 * @param funcAddr The function address.
 * @param resultLenght The addr of the length of the result.
 * @param result The result addr.
 */
PLCAPI void pl_lookup_symbol(void* func, size_t* resultLenght, const char*** result);

PLCAPI void pl_free_lookup_result(const char** result);

} // namespace pl::symbol_provider
