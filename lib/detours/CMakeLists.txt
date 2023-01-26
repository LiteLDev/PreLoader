# file from https://github.com/0xeb/detours-cmake
# Detours
project(detours)

set(DETOURS_SOURCE detours/src)

add_library(detours STATIC
        ${DETOURS_SOURCE}/creatwth.cpp
        ${DETOURS_SOURCE}/detours.cpp
        ${DETOURS_SOURCE}/detours.h
        ${DETOURS_SOURCE}/detver.h
        ${DETOURS_SOURCE}/disasm.cpp
        ${DETOURS_SOURCE}/disolarm.cpp
        ${DETOURS_SOURCE}/disolarm64.cpp
        ${DETOURS_SOURCE}/disolia64.cpp
        ${DETOURS_SOURCE}/disolx64.cpp
        ${DETOURS_SOURCE}/disolx86.cpp
        ${DETOURS_SOURCE}/image.cpp
        ${DETOURS_SOURCE}/modules.cpp
        ${DETOURS_SOURCE}/uimports.cpp)

# This file is included and not compiled on its own
set_property(
        SOURCE ${DETOURS_SOURCE}/uimports.cpp
        APPEND PROPERTY HEADER_FILE_ONLY true)

target_compile_options(
        detours PRIVATE
        /W4 /WX /Zi /Gy /Gm- /Zl
        "$<$<CONFIG:Debug>:/Od;/MDd>"
        "$<$<CONFIG:Release>:/O2;/MD>"
)
target_include_directories(detours PRIVATE ${DETOURS_SOURCE})
