include(FetchContent)

FetchContent_Declare(
    quickjs
    GIT_REPOSITORY git@github.com:quickjs-ng/quickjs.git
    GIT_TAG        9d15fb60b67c45fd0de413bb49e48f8dacebac16
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable( quickjs )

# add_library(quickjs IMPORTED)

# add_library(quickjsb)
# add_library( quickjs::quickjs ALIAS libquickjs )
# add_library( quickjs::quickjs ALIAS quickjs )

# Fix: Alias 'qjs' to your preferred modern scoped namespace
if(TARGET qjs)
    add_library(quickjs::quickjs ALIAS qjs)
endif()

# Map the companion utility library target as well
if(TARGET qjs-libc)
    add_library(quickjs::quickjs-libc ALIAS qjs-libc)
endif()