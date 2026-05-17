include(FetchContent)

FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG        v5.4.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable(lua)


# add_library(lua STATIC)
# add_library(lua SHARED IMPORTED GLOBAL)
if (EMSCRIPTEN)
    add_library(lua STATIC)
else()
    add_library(lua SHARED)
endif()


# 3. Configure the include directories with proper scopes (for us, both should be the same i guess...)
target_include_directories(lua
    PRIVATE 
        # Path for internal .c files to find their own internal headers
        "${lua_SOURCE_DIR}"
        
    PUBLIC 
        # Path containing public headers that other targets need to see
        # (e.g., if mainapp.cpp needs to #include "lua.h")
        # "${lua_SOURCE_DIR}/include" 
        "${lua_SOURCE_DIR}"
)


# basic version - works!
# target_sources(
#     lua
#     PUBLIC
#     ${lua_SOURCE_DIR}/....c                
# )



# Base source files
# set(lua_SOURCES
#     ${lua_SOURCE_DIR}/...c
# )


# Why file(GLOB) is Dangerous
# CMake generates build files (like Makefiles or Ninja files) based on your CMakeLists.txt.
#   If you use file(GLOB), CMake takes a snapshot of the files at that exact second.
#   If you later add a new .c file to that directory using your IDE or terminal, CMake will not know it exists.
#   Because CMakeLists.txt itself didn't change, your build tool (Ninja/Make) will skip re-running CMake. Your project will fail to compile or link because the new file is missing from the build.

#The Correct Way: 
# List Files ExplicitlyThe recommended approach is to explicitly list your source files using target_sources(). This guarantees that if you add or remove a file, you modify the CMakeLists.txt, forcing CMake to automatically regenerate your build files on the next build.

# The Only Exception: CONFIGURE_DEPENDS
# If you absolutely must use a glob pattern (for example, if a third-party directory has dozens of files that change constantly), you must use the CONFIGURE_DEPENDS flag. This flag tells the underlying build tool to check the directory for new files on every single build.
# (Note: This requires CMake 3.12 or newer).
if ( FALSE )
    file(GLOB lua_SOURCES CONFIGURE_DEPENDS "${lua_SOURCE_DIR}/*.c")
    # Apply to target
    target_sources(lua PRIVATE ${lua_SOURCES})
elseif( FALSE )
    target_sources(
        lua 
        PRIVATE 
        ${lua_SOURCE_DIR}/lapi.c
        # ${lua_SOURCE_DIR}/lauxlib.c
        # ${lua_SOURCE_DIR}/lbaselib.c
        ${lua_SOURCE_DIR}/lcode.c
        # ${lua_SOURCE_DIR}/lcorolib.c
        ${lua_SOURCE_DIR}/lctype.c
        # ${lua_SOURCE_DIR}/ldblib.c
        ${lua_SOURCE_DIR}/ldebug.c
        ${lua_SOURCE_DIR}/ldo.c
        ${lua_SOURCE_DIR}/ldump.c
        ${lua_SOURCE_DIR}/lfunc.c
        ${lua_SOURCE_DIR}/lgc.c
        # ${lua_SOURCE_DIR}/linit.c
        # ${lua_SOURCE_DIR}/liolib.c
        ${lua_SOURCE_DIR}/llex.c
        # ${lua_SOURCE_DIR}/lmathlib.c
        ${lua_SOURCE_DIR}/lmem.c
        # ${lua_SOURCE_DIR}/loadlib.c
        ${lua_SOURCE_DIR}/lobject.c
        ${lua_SOURCE_DIR}/lopcodes.c
        # ${lua_SOURCE_DIR}/loslib.c
        ${lua_SOURCE_DIR}/lparser.c
        ${lua_SOURCE_DIR}/lstate.c
        ${lua_SOURCE_DIR}/lstring.c
        ${lua_SOURCE_DIR}/lstrlib.c
        ${lua_SOURCE_DIR}/ltable.c
        # ${lua_SOURCE_DIR}/ltablib.c
        # ${lua_SOURCE_DIR}/ltests.c
        ${lua_SOURCE_DIR}/ltm.c
        # ${lua_SOURCE_DIR}/lua.c
        ${lua_SOURCE_DIR}/lundump.c
        # ${lua_SOURCE_DIR}/lutf8lib.c
        ${lua_SOURCE_DIR}/lvm.c
        ${lua_SOURCE_DIR}/lzio.c
        # ${lua_SOURCE_DIR}/onelua.c
        # ${lua_SOURCE_DIR}/
    )
else()
    # block()

        # target_compile_definitions(lua
        #     #PRIVATE MAKE_LIB=1 LUA_CORE
        #     PRIVATE MAKE_LIB=1 
        #     PUBLIC  LUA_BUILD_AS_DLL
        # )

        # Apply the definition ONLY to this specific target
        # LUA_CORE + LUA_BUILD_AS_DLL: on Windows, MSVC only generates the import
        # lib (lua.lib) when the DLL exports symbols. LUA_BUILD_AS_DLL enables
        # __declspec(dllexport/dllimport) in luaconf.h; LUA_CORE makes this DLL
        # use dllexport (consumers that link against it get dllimport via PUBLIC).
        target_compile_definitions(lua PRIVATE MAKE_LIB=1)
        # LUA_BUILD_AS_DLL uses __declspec which is MSVC/Windows-only.
        # Emscripten and other non-Windows targets must not see it.
        if(WIN32)
            target_compile_definitions(lua PUBLIC LUA_BUILD_AS_DLL)
        endif()
        target_sources(
            lua
            PRIVATE
            ${lua_SOURCE_DIR}/onelua.c
        )
    # endblock()
endif()

add_library(lua_lib::lua_lib ALIAS lua)

