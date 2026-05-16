include(FetchContent)

FetchContent_Declare(
    #luajit_lib
    libluajit
    GIT_REPOSITORY https://github.com/WohlSoft/LuaJIT.git
    GIT_TAG        v2.1
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable( libluajit )

# add_library(luajit_lib IMPORTED)

# add_library(luajit_lib)
add_library( lua_lib::lua_lib ALIAS libluajit )
