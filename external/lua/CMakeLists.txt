
# 1. Extract the strings from the global list using real index numbers
list(GET LUA_MODE_CHOICES 0 MODE_LUAJIT_ORIGINAL)  # Luajit_OriginalExternalAdd
list(GET LUA_MODE_CHOICES 1 MODE_LUAJIT_WOHLSOFT)  # Luajit_WohlsoftCmakeWrapper
list(GET LUA_MODE_CHOICES 2 MODE_LUA_VANILLA)   # VanillaLua

# 2. Validate user command-line input
list(FIND LUA_MODE_CHOICES "${LUA_MODE}" LUA_MODE_INDEX)
if(LUA_MODE_INDEX EQUAL -1)
    message(FATAL_ERROR "Unsupported LUA_MODE: '${LUA_MODE}'. Choose from: ${LUA_MODE_CHOICES}")
endif()

# 3. Use the helper variables for your conditional logic
if(LUA_MODE STREQUAL MODE_LUAJIT_ORIGINAL)
    message(STATUS "LuaJit using the original repo via homebrewed ExternalProject_Add()")
    add_subdirectory(luajit/LuaJit_OriginalRepo_CMakeExternal)
elseif(LUA_MODE STREQUAL MODE_LUAJIT_WOHLSOFT)
    message(STATUS "LuaJit using Wohlsoft's CMake wrapper")
    add_subdirectory(luajit/LuaJit_CMakeWrapperWohlSoft)
elseif(LUA_MODE STREQUAL MODE_LUA_VANILLA)
    message(STATUS "Vanilla Lua via homebrewed ExternalProject_Add()")
    add_subdirectory(Lua_CMakeExternal)
else()
    message(ERROR "invalid Lua lib/build mode selected!")
endif()
