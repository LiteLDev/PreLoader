#pragma once

#ifdef PRELOADER_EXPORT
#define PLAPI [[maybe_unused]] __declspec(dllexport)
#else
#define PLAPI [[maybe_unused]] __declspec(dllimport)
#endif

#define PLCAPI extern "C" PLAPI
