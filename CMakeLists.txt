cmake_minimum_required(VERSION 3.21) # Note that TARGET_RUNTIME_DLLS requires CMake 3.21+ ... for copying SDL3.dll on Windows after build
project(start2d-noCPM-project)

# # 1. Set the list of allowed values
# set(MY_MODE_CHOICES "Development" "Staging" "Production")
# # 2. Define the cache variable with a default value
# set(MY_MODE "Development" CACHE STRING "Select the build deployment mode")
# # 3. Attach the STRINGS property so GUI tools understand it's an enum
# set_property(CACHE MY_MODE PROPERTY STRINGS ${MY_MODE_CHOICES})

# 1. Set the list of allowed values
# set(LUA_MODE_CHOICES "Luajit_OriginalExternalAdd" "Luajit_WohlsoftCmakeWrapper" "VanillaLua" CACHE INTERNAL "internal (invisible) possible choices for the lua lib / lua lib build mode")
set(LUA_MODE_CHOICES "Luajit_OriginalExternalAdd" "Luajit_WohlsoftCmakeWrapper" "VanillaLua" CACHE INTERNAL "")

# 2. Define the cache variable with a default value
set(LUA_MODE "Luajit_OriginalExternalAdd" CACHE STRING "Select a lua library / lua library build method")

# 3. Attach the STRINGS property so GUI tools understand it's an enum
set_property(CACHE LUA_MODE PROPERTY STRINGS ${LUA_MODE_CHOICES})

message(STATUS "valid choices: ${LUA_MODE_CHOICES}")
# message(WARNING "valid choices: ${LUA_MODE_CHOICES}")

message(STATUS "chosen mode: '${LUA_MODE}'")

add_subdirectory(external)
add_subdirectory(src)
