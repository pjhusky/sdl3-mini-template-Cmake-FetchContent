include(FetchContent)

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    # GIT_TAG        v3.3.0   # issues with emscripten - error: no member named 'construct' in 'sol::optional<T &>'
    GIT_TAG        develop
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable(sol2)

add_library( sol3::sol3 ALIAS sol2 )
