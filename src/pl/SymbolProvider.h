#pragma once

#include "pl/internal/Macro.h"

namespace pl::symbol_provider {

void init();

PLCAPI void* pl_resolve_symbol(const char* symbolName);

/**
 * @brief Get the symbol name of a function address.
 *
 * @param funcAddr [in] The function address.
 * @param resultLength [out, nullable] The addr of the length of the result. 0 if the function is not found.
 * @return result The result addr. The result is a null-terminated array of strings. nullptr
 * if the function is not found.
 */
PLCAPI const char* const* pl_lookup_symbol(void* func, size_t* resultLength);

PLCAPI void pl_free_lookup_result(const char* const* result);

} // namespace pl::symbol_provider
