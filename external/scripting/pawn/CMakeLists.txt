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

if(WIN32)
    # ---- patch compiler/CMakeLists.txt ----------------------------------------
    set(_pawn_compiler_cmake "${pawn_SOURCE_DIR}/compiler/CMakeLists.txt")
    file(READ "${_pawn_compiler_cmake}" _content)

    # Patch 1: pawnc SHARED → STATIC
    # On Windows a DLL cannot have unresolved symbols at link time, so pawnc.dll
    # cannot call the pc_* callbacks that the host is supposed to provide.
    # Building as STATIC lets pawn_host*.c satisfy those references at EXE link time.
    string(REPLACE
        [=[ADD_LIBRARY(pawnc SHARED ${PAWNCC_SRCS})]=]
        [=[ADD_LIBRARY(pawnc STATIC ${PAWNCC_SRCS})]=]
        _content "${_content}"
    )

    # Patch 2: remove pawncc.rc — it references ../bin/pawn.ico which is absent
    # from the git repo (binary-distribution only).
    string(REPLACE
        [=[SET(PAWNCC_SRCS ${PAWNCC_SRCS} pawncc.rc)]=]
        [=[# pawncc.rc excluded: bin/pawn.ico is absent from the git repo]=]
        _content "${_content}"
    )

    file(WRITE "${_pawn_compiler_cmake}" "${_content}")
    unset(_pawn_compiler_cmake)
    unset(_content)

    # ---- patch amx/CMakeLists.txt ---------------------------------------------
    set(_pawn_amx_cmake "${pawn_SOURCE_DIR}/amx/CMakeLists.txt")
    file(READ "${_pawn_amx_cmake}" _content)

    # Patch 3: amx SHARED → STATIC
    # The amx DLL does not __declspec(dllexport) its API (amx_Init, amx_Exec,
    # amx_Register, aux_*, …) so the import library is empty and the EXE linker
    # reports LNK2019 for every AMX function.  STATIC avoids the export problem.
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
