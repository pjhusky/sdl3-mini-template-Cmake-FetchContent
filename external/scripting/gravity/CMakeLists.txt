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

# This binds the internal 'gravity' target to a global 'gravity::gravity' name
add_library(gravity::gravity ALIAS gravityapi)

# This forces anyone who links to gravity (like start2d-noCPM) to use C++20
target_compile_features(gravityapi INTERFACE cxx_std_20)
