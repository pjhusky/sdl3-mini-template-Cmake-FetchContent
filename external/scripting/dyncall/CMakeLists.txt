
include(FetchContent)

# -------------------------------------------------------------------
# 1. Clean Native Flag Fixes (Resolves error RC2135 without file edits)
# -------------------------------------------------------------------
# if(MSVC)
#     # Automatically inject the pawn-src folder into the Resource Compiler includes
#     set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /i \"${CMAKE_BINARY_DIR}/_deps/pawn-src\"")
# endif()



# Silence deprecation policy logs cascading from older third-party submodules
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW) # Silences legacy package/project version warnings
endif()

# # Fixes the Windows Resource Compiler relative path lookup dynamically
# if(MSVC)
#     # Appends the required relative base folder paths to the global RC include properties
#     link_directories(${CMAKE_BINARY_DIR}/_deps/pawn-src)
#     add_compile_definitions(RC_INVOKED)
    
#     # Inject directory into CMAKE_RC_FLAGS for targeted toolchain visibility
#     set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /i \"${CMAKE_BINARY_DIR}/_deps/pawn-src/compiler\" /i \"${CMAKE_BINARY_DIR}/_deps/pawn-src\"")
# endif()



# Downloads pre-packaged source archive directly (Faster, no tool required)
FetchContent_Declare(
    dyncall
    URL      https://dyncall.org/r1.4/dyncall-1.4.tar.gz
    URL_HASH SHA256=14437dbbef3b6dc92483f6507eaf825ab97964a89eecae8cb347a6bec9c32aae
    OVERRIDE_FIND_PACKAGE  # <-- Forces find_package(Dyncall) to match this in-tree version
)

# Force CMake 4+ to accept older dependency syntax
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)



FetchContent_MakeAvailable(dyncall)


# 2. Expose the modern alias namespaces expected by your executable target
if(TARGET dyncall_s)
    add_library(dyncall::dyncall ALIAS dyncall_s)
elseif(TARGET dyncall)
    add_library(dyncall::dyncall ALIAS dyncall)
endif()
