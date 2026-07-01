cmake_minimum_required(VERSION 3.18)
include(FetchContent)

# CMP0169 OLD: allow FetchContent_Populate so we can patch the upstream
# CMakeLists.txt before add_subdirectory processes it.
cmake_policy(SET CMP0169 OLD)

# Force CMake 4+ to accept older dependency syntax from pawn's own files
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_Declare(
    pawn
    GIT_REPOSITORY https://codeberg.org/compuphase/pawn.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

# Download-only — no add_subdirectory yet so we can patch first
FetchContent_GetProperties(pawn)
if(NOT pawn_POPULATED)
    FetchContent_Populate(pawn)
endif()

# ---- patch compiler/CMakeLists.txt (all platforms) ---------------------------
set(_pawn_compiler_cmake "${pawn_SOURCE_DIR}/compiler/CMakeLists.txt")
file(READ "${_pawn_compiler_cmake}" _content)

# Patch 1: pawnc SHARED → STATIC (needed everywhere)
# pawnc calls pc_* callbacks that the host (pawn_host*.c) provides.  A shared
# library cannot have unresolved symbols at link time, and on Linux the
# pc_* symbols compiled into the exe have hidden visibility (due to pawnc's
# own -fvisibility=hidden propagating via PUBLIC), so the DSO linker reports
# "hidden symbol referenced by DSO".  STATIC defers resolution to final link.
string(REPLACE
    [=[ADD_LIBRARY(pawnc SHARED ${PAWNCC_SRCS})]=]
    [=[ADD_LIBRARY(pawnc STATIC ${PAWNCC_SRCS})]=]
    _content "${_content}"
)

# Patch 2: remove the upstream Apple i386 override (all platforms read/write pass)
# The pawn compiler CMakeLists.txt forces CMAKE_OSX_ARCHITECTURES to "i386" on
# Apple for historical reasons (pawncc was always a 32-bit host tool).  macOS
# 10.15 (Catalina) dropped 32-bit support entirely, so libm/libSystem/libSystem
# no longer ship i386 slices → link failure on any modern macOS runner.
string(REPLACE
    [=[SET(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "" FORCE)]=]
    [=[# CMAKE_OSX_ARCHITECTURES i386 removed: macOS 10.15+ dropped 32-bit support]=]
    _content "${_content}"
)

file(WRITE "${_pawn_compiler_cmake}" "${_content}")
unset(_pawn_compiler_cmake)
unset(_content)

if(WIN32)
    # ---- patch compiler/CMakeLists.txt (Windows-only) ------------------------
    set(_pawn_compiler_cmake "${pawn_SOURCE_DIR}/compiler/CMakeLists.txt")
    file(READ "${_pawn_compiler_cmake}" _content)

    # Patch 3: remove pawncc.rc — it references ../bin/pawn.ico which is absent
    # from the git repo (binary-distribution only).
    string(REPLACE
        [=[SET(PAWNCC_SRCS ${PAWNCC_SRCS} pawncc.rc)]=]
        [=[# pawncc.rc excluded: bin/pawn.ico is absent from the git repo]=]
        _content "${_content}"
    )

    file(WRITE "${_pawn_compiler_cmake}" "${_content}")
    unset(_pawn_compiler_cmake)
    unset(_content)

    # ---- patch amx/CMakeLists.txt (Windows-only) -----------------------------
    set(_pawn_amx_cmake "${pawn_SOURCE_DIR}/amx/CMakeLists.txt")
    file(READ "${_pawn_amx_cmake}" _content)

    # Patch 4: amx SHARED → STATIC
    # The amx DLL does not __declspec(dllexport) its API (amx_Init, amx_Exec,
    # amx_Register, aux_*, …) so the import library is empty and the EXE linker
    # reports LNK2019 for every AMX function.  STATIC avoids the export problem.
    # On Linux amx/CMakeLists.txt already uses STATIC unconditionally.
    string(REPLACE
        [=[ADD_LIBRARY(amx SHARED ${AMXLIB_SRCS})]=]
        [=[ADD_LIBRARY(amx STATIC ${AMXLIB_SRCS})]=]
        _content "${_content}"
    )

    file(WRITE "${_pawn_amx_cmake}" "${_content}")
    unset(_pawn_amx_cmake)
    unset(_content)
endif()

add_subdirectory("${pawn_SOURCE_DIR}" "${pawn_BINARY_DIR}")

# Safety net: if the upstream used a different string form for the i386 cache
# set (e.g. lowercase, different quoting), the file patch above silently no-ops.
# Setting OSX_ARCHITECTURES "" on the targets here overrides CMAKE_OSX_ARCHITECTURES
# at the target level (CMake evaluates this at generate time, before any -arch flag
# is emitted), so both compile and link steps drop the i386 constraint.
# Targets affected: pawnc (static lib), pawncc (compiler exe), stategraph (utility exe).
if(APPLE)
    foreach(_pawn_t pawnc pawncc stategraph)
        if(TARGET ${_pawn_t})
            set_target_properties(${_pawn_t} PROPERTIES OSX_ARCHITECTURES "")
        endif()
    endforeach()
    unset(_pawn_t)
endif()

# The upstream pawnc CMakeLists.txt uses PUBLIC -fvisibility=hidden, which
# propagates to every consumer (including our main exe).  That makes the
# pc_* symbols in pawn_host_mem.c hidden, so the linker sees them as hidden
# even when resolving references from pawnc (now static).  Demote to PRIVATE
# so the flag stays internal to the pawnc compilation units.
if(TARGET pawnc)
    get_target_property(_pawnc_opts pawnc INTERFACE_COMPILE_OPTIONS)
    if(_pawnc_opts)
        list(REMOVE_ITEM _pawnc_opts "-fvisibility=hidden")
        set_target_properties(pawnc PROPERTIES INTERFACE_COMPILE_OPTIONS "${_pawnc_opts}")
    endif()
    unset(_pawnc_opts)
endif()

# Force 64-bit cells on 64-bit hosts.  amx.h auto-detects via __SIZEOF_POINTER__
# but MSVC does not define that macro, so it defaults to PAWN_CELL_SIZE=32.
# Without this, AMX_MAGIC (runtime) is 0xf1e0 but the compiler with -C64 writes
# 0xf1e1, causing amx_Init to return AMX_ERR_FORMAT.  PUBLIC propagates the
# define through pawn_bundle to the main executable (mini_pawn.h, pawn_host_mem.c).
if(TARGET amx)
    target_compile_definitions(amx PUBLIC PAWN_CELL_SIZE=64)
endif()
if(TARGET pawnc)
    target_compile_definitions(pawnc PUBLIC PAWN_CELL_SIZE=64)
endif()

if(NOT TARGET pawn::pawn)
    if(NOT TARGET pawnc OR NOT TARGET amx)
        message(FATAL_ERROR "Expected targets 'pawnc' and 'amx' from the Pawn repository were not found.")
    endif()
    # Bundle compiler + runtime into one convenient target with include dirs
    add_library(pawn_bundle INTERFACE)
    target_link_libraries(pawn_bundle INTERFACE pawnc amx)
    target_include_directories(pawn_bundle INTERFACE
        "${pawn_SOURCE_DIR}/compiler"
        "${pawn_SOURCE_DIR}/amx"
    )
    add_library(pawn::pawn ALIAS pawn_bundle)
endif()

message(STATUS "Pawn source: ${pawn_SOURCE_DIR}")
message(STATUS "Pawn build:  ${pawn_BINARY_DIR}")
