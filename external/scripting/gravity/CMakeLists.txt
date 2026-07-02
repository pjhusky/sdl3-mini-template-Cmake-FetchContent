include(FetchContent)

FetchContent_Declare(
    gravity
    GIT_REPOSITORY https://github.com/marcobambini/gravity.git
    GIT_TAG        0.9.7
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)



# # 1. Set your option as a forced cache variable
# set(BUILD_CLI OFF CACHE BOOL "Disable gravity CLI build" FORCE)
#
# # 2. Hide the option from the standard CMake GUI view
# mark_as_advanced(BUILD_CLI)


# Set as a normal variable (no CACHE or FORCE needed)
# This overrides the option only for gravity and its children
set(BUILD_CLI OFF) 


# Force CMake 4+ to accept older dependency syntax
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_MakeAvailable( gravity )

# This binds the internal 'gravity' target to a global 'gravity::gravity' name.
# We use the static target (gravityapi_s), not the shared gravityapi.dll: on
# MSVC, gravity's foundation class globals (gravity_class_int, gravity_class_float,
# etc., used by macros like VALUE_ISA_INT/VALUE_FROM_INT) are declared as plain
# `extern` with no dllexport/dllimport annotation, so MSVC never exports them from
# the DLL and any consumer using those macros fails to link. Static linking sidesteps
# that gap entirely, and matches how gravity's own CLI links itself.
add_library(gravity::gravity ALIAS gravityapi_s)

# This forces anyone who links to gravity (like start2d-noCPM) to use C++20
target_compile_features(gravityapi_s INTERFACE cxx_std_20)

# bzero() is a POSIX function absent from the Windows CRT.  gravity_lexer.c
# calls it without including <strings.h>, and GCC 14+ promotes the resulting
# implicit-function-declaration warning to a hard error in C99+ modes.
# Write a one-liner compat header and force-include it for all gravity TUs;
# no gravity source file needs to be modified.
if(WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(_bzero_shim "${gravity_BINARY_DIR}/bzero_compat.h")
    file(WRITE "${_bzero_shim}"
        "#pragma once\n#include <string.h>\n#define bzero(s,n) memset((s),0,(n))\n")
    # gravity builds both a shared (gravityapi) and a static (gravityapi_s) target;
    # apply the bzero shim to whichever variants exist.
    foreach(_gravity_tgt gravityapi gravityapi_s)
        if(TARGET ${_gravity_tgt})
            target_compile_options(${_gravity_tgt} PRIVATE -include "${_bzero_shim}")
        endif()
    endforeach()
    unset(_bzero_shim)
    unset(_gravity_tgt)
endif()
