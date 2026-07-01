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

# ---- patch amx/amx.c (all platforms) -----------------------------------------
set(_pawn_amx_c "${pawn_SOURCE_DIR}/amx/amx.c")
file(READ "${_pawn_amx_c}" _content)

# Patch 3: fix upstream bug — strrchr returns char *, not char.
# Clang 15+ / Xcode 16+ promotes this pointer-to-integer conversion from warning
# to hard error.  The missing '*' causes four cascading errors (dereference,
# asprintf type mismatch, dlopen, free).  This is a genuine upstream defect worth
# reporting; patching locally is cleaner than suppressing -Wint-conversion broadly.
string(REPLACE
    [=[char ptr=strrchr(pathbuf,'/')]=]
    [=[char *ptr=strrchr(pathbuf,'/')]=]
    _content "${_content}"
)

file(WRITE "${_pawn_amx_c}" "${_content}")
unset(_pawn_amx_c)
unset(_content)

# ---- create amx/sclinux.h stub if absent (all platforms) --------------------
# amx.h line 33 does #include "sclinux.h" (after Patch 4 below).
# sclinux.h is absent from the current pawn main branch; create a minimal shim
# in amx/ next to amx.h so the quoted include always resolves without needing
# any -I path adjustments.  The file is only written when missing so a future
# pawn commit that ships it again takes precedence.
set(_pawn_sclinux "${pawn_SOURCE_DIR}/amx/sclinux.h")
if(NOT EXISTS "${_pawn_sclinux}")
    file(WRITE "${_pawn_sclinux}"
"/* sclinux.h — auto-generated stub: file absent from this pawn checkout.
   Provides the Unix string-comparison aliases used by amx.h on GCC/Clang. */
#pragma once
#if defined __linux__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
#  include <strings.h>
#  ifndef stricmp
#    define stricmp(a,b)    strcasecmp((a),(b))
#  endif
#  ifndef strnicmp
#    define strnicmp(a,b,n) strncasecmp((a),(b),(n))
#  endif
#  ifndef pstrcmp
#    define pstrcmp(a,b)    strcasecmp((a),(b))
#  endif
#endif
")
endif()
unset(_pawn_sclinux)

# ---- patch amx/amx.h (all platforms) ----------------------------------------
set(_pawn_amx_h "${pawn_SOURCE_DIR}/amx/amx.h")
file(READ "${_pawn_amx_h}" _content)

# Patch 4: fix <sclinux.h> → "sclinux.h"
# sclinux.h lives in amx/ alongside amx.h.  The upstream amx/CMakeLists.txt
# exposes amx/ only PRIVATE (for building amx itself), so any consumer that
# includes amx.h without amx/ in its own -I path cannot find sclinux.h.
# A same-directory quoted include sidesteps the -I path entirely: the
# preprocessor resolves "sclinux.h" relative to the directory of the file
# containing the directive (amx.h → amx/), so it always finds amx/sclinux.h
# regardless of which file includes amx.h or what -I flags the consumer has.
string(REPLACE
    [=[  #include <sclinux.h>]=]
    [=[  #include "sclinux.h"]=]
    _content "${_content}"
)

file(WRITE "${_pawn_amx_h}" "${_content}")
unset(_pawn_amx_h)
unset(_content)

# ---- patch amx/CMakeLists.txt (all platforms) --------------------------------
set(_pawn_amx_cmake "${pawn_SOURCE_DIR}/amx/CMakeLists.txt")
file(READ "${_pawn_amx_cmake}" _content)

# Patch 5: amx SHARED → STATIC (needed on Windows and macOS; Linux is already STATIC)
# As a shared library, amx cannot have unresolved symbols at link time:
#   Windows — amx DLL has no __declspec(dllexport), so the import library is
#             empty and the EXE linker reports LNK2019 for every AMX function.
#   macOS   — amx_Swap64 (and others) are undefined in the dylib at link time
#             because they are only satisfied later by the host executable.
# STATIC defers all symbol resolution to the final executable link.
# On Linux amx/CMakeLists.txt already uses STATIC; string(REPLACE) no-ops safely.
string(REPLACE
    [=[ADD_LIBRARY(amx SHARED ${AMXLIB_SRCS})]=]
    [=[ADD_LIBRARY(amx STATIC ${AMXLIB_SRCS})]=]
    _content "${_content}"
)

file(WRITE "${_pawn_amx_cmake}" "${_content}")
unset(_pawn_amx_cmake)
unset(_content)

if(WIN32)
    # ---- patch compiler/CMakeLists.txt (Windows-only) ------------------------
    set(_pawn_compiler_cmake "${pawn_SOURCE_DIR}/compiler/CMakeLists.txt")
    file(READ "${_pawn_compiler_cmake}" _content)

    # Patch 6: remove pawncc.rc — it references ../bin/pawn.ico which is absent
    # from the git repo (binary-distribution only).
    string(REPLACE
        [=[SET(PAWNCC_SRCS ${PAWNCC_SRCS} pawncc.rc)]=]
        [=[# pawncc.rc excluded: bin/pawn.ico is absent from the git repo]=]
        _content "${_content}"
    )

    file(WRITE "${_pawn_compiler_cmake}" "${_content}")
    unset(_pawn_compiler_cmake)
    unset(_content)
endif()

add_subdirectory("${pawn_SOURCE_DIR}" "${pawn_BINARY_DIR}")

# We only need pawnc (compiler library) and amx (runtime library).
# All other pawn targets — pawncc, stategraph, pawndisasm, amxArgs, amxFloat,
# amxString, … — are excluded from the default build on every platform.
# This avoids compiling extension modules that have known issues with strict
# modern compilers (e.g. Clang 15+/Xcode 16+ promotes pointer↔integer implicit
# conversions from warning to hard error).
# On Apple, OSX_ARCHITECTURES "" is also set on the two needed targets to
# override the upstream's CMAKE_OSX_ARCHITECTURES "i386" cache entry, which
# would otherwise prevent them from compiling on macOS 10.15+ (no i386 libc).
set(_pawn_needed pawnc amx)
foreach(_pawn_dir
        "${pawn_SOURCE_DIR}/compiler"
        "${pawn_SOURCE_DIR}/amx"
)
    get_property(_pawn_dir_targets
        DIRECTORY "${_pawn_dir}"
        PROPERTY BUILDSYSTEM_TARGETS
    )
    foreach(_pawn_t ${_pawn_dir_targets})
        if("${_pawn_t}" IN_LIST _pawn_needed)
            if(APPLE)
                set_target_properties(${_pawn_t} PROPERTIES OSX_ARCHITECTURES "")
            endif()
        else()
            set_target_properties(${_pawn_t} PROPERTIES EXCLUDE_FROM_ALL TRUE)
        endif()
    endforeach()
endforeach()
unset(_pawn_dir)
unset(_pawn_dir_targets)
unset(_pawn_t)
unset(_pawn_needed)

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
    # amx.h includes sclinux.h which lives in amx/ alongside amx.h.
    # The upstream amx/CMakeLists.txt adds amx/ only PRIVATE (for building amx)
    # but not INTERFACE (for consumers).  Expose it so any target including amx.h
    # can resolve sclinux.h even without the Patch 4 amx.h fix.
    target_include_directories(amx INTERFACE "${pawn_SOURCE_DIR}/amx")
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
