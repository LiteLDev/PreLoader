cmake_minimum_required(VERSION 3.24)

add_subdirectory(lib/demangler)
add_subdirectory(lib/detours)
add_subdirectory(lib/fmt)
add_subdirectory(lib/json)
add_subdirectory(lib/parallel_hashmap)
add_subdirectory(lib/raw_pdb)

project(PreLoader)

set(CMAKE_CXX_STANDARD 17)

file(
        GLOB_RECURSE PROJECT_SRC
        ${PROJECT_SOURCE_DIR}/src/*.cpp
)

add_library(PreLoader SHARED ${PROJECT_SRC})

target_include_directories(
        PreLoader PRIVATE
        ${raw_pdb_SOURCE_DIR}/src/
        ${phmap_SOURCE_DIR}/
        ${detours_SOURCE_DIR}/detours/src/
        ${FMT_SOURCE_DIR}/include/
        ${nlohmann_json_SOURCE_DIR}/include/
        ${Demangler_SOURCE_DIR}/include/
)

target_include_directories(
        PreLoader PUBLIC
        ${PROJECT_SOURCE_DIR}/include/
)

target_compile_definitions(PreLoader PRIVATE NDEBUG _WINDOWS _USRDLL _WINDLL _UNICODE UNICODE _AMD64_ WIN32_LEAN_AND_MEAN NOMINMAX PRELOADER_EXPORT)

target_compile_options(
        PreLoader PRIVATE
        /permissive- /GS /GL /W0 /Gy /Zc:wchar_t /Zi /Gm- /sdl /Zc:inline /fp:precise
        /errorReport:prompt /WX- /Zc:forScope /Gd /Oi /FC /EHsc /nologo /diagnostics:column
        "$<$<CONFIG:Debug>:/Od;/MDd>"
        "$<$<CONFIG:Release>:/O2;/MD>"
)

target_link_options(
        PreLoader PRIVATE
        /MANIFEST /LTCG:incremental /NXCOMPAT /DEBUG /DLL /MACHINE:X64 /OPT:REF /INCREMENTAL:NO
        /SUBSYSTEM:WINDOWS /MANIFESTUAC:NO /OPT:ICF /ERRORREPORT:PROMPT /NOLOGO /TLBID:1
)

target_link_libraries(PreLoader PRIVATE raw_pdb phmap detours nlohmann_json Demangler)
