#pragma once

#include "pl/internal/Macro.h" // IWYU pragma: export

#ifdef __cplusplus
namespace pl {
#endif

PLCAPI char const* pl_mod_manager_name;
PLCAPI char        pl_color_log;
PLCAPI int         pl_log_level;
PLCAPI char const* pl_log_path;
PLCAPI char const* pl_mods_path;

#ifdef __cplusplus
}
#endif
