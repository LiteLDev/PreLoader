#pragma once

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "pl/internal/Macro.h"

#ifdef __cplusplus
namespace pl::symbol_provider {
#endif

void init();

/**
 * @brief Resolve a symbol name to a function address.
 *
 * @param symbolName [in] The symbol name.
 * @return result The function address. nullptr if the function is not found.
 */
PLCAPI void* pl_resolve_symbol(const char* symbolName);

/**
 * @brief Resolve a symbol name to a function address.
 *
 * @param symbolName [in] The symbol name.
 * @return result The function address. nullptr if the function is not found.
 *
 * @note This function will not print error message if the function is not found.
 */
PLCAPI void* pl_resolve_symbol_silent(const char* symbolName);

/**
 * @brief Resolve a symbol name to a function address.
 *
 * @param symbolName [in] The symbol name.
 * @param symbolName [in] The name size.
 * @return result The function address. nullptr if the function is not found.
 *
 * @note This function will not print error message if the function is not found.
 */
PLCAPI void* pl_resolve_symbol_silent_n(const char* symbolName, size_t size);

/**
 * @brief Get the symbol name of a function address.
 *
 * @param funcAddr [in] The function address.
 * @param resultLength [out, nullable] The addr of the length of the result. 0 if the function is not found.
 * @return result The result addr. The result is a null-terminated array of strings. nullptr
 * if the function is not found. The result should be freed by pl_free_lookup_result.
 */
PLCAPI const char* const* pl_lookup_symbol(void* func, size_t* resultLength);

PLCAPI const char* const* pl_lookup_symbol_disp(void* func, size_t* resultLength, unsigned int* displacement);

/**
 * @brief Free the result of pl_lookup_symbol.
 *
 * @param result [in] The result of pl_lookup_symbol to free.
 */
PLCAPI void pl_free_lookup_result(const char* const* result);

#ifdef __cplusplus
} // namespace pl::symbol_provider
#endif
