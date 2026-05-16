# cmake_language(DEFER) requires 3.19+
# cmake_path requires 3.20+
cmake_minimum_required(VERSION 3.20)

project(MyLuaApp LANGUAGES C CXX)

include(FetchContent)
include(ExternalProject)

set(LIB_TO_BUILD luajit)

# 1. Declare the official repository
FetchContent_Declare(
    ${LIB_TO_BUILD}
    GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
    GIT_TAG        v2.1
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

# 2. Populate the source directory at configure time.
#    FetchContent_MakeAvailable skips add_subdirectory when there is no
#    top-level CMakeLists.txt (official LuaJIT has none), so this is safe.
FetchContent_MakeAvailable(${LIB_TO_BUILD})

# Shorten paths using our variable trick
set(LUAJIT_SRC_DIR  ${${LIB_TO_BUILD}_SOURCE_DIR})
set(LUAJIT_BIN_DIR  ${${LIB_TO_BUILD}_BINARY_DIR})


if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  message(STATUS "Using MSVC compiler")
  # Add MSVC-specific flags here (e.g., /W4)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # MinGW identifies itself as "GNU" because it is a GCC port
  message(STATUS "Using MinGW (GCC) compiler")
  # Add GCC-specific flags here (e.g., -Wall)
endif()

message(WARNING "We should check not only for WIN32 but also for the compiler (MSVC vs. MINGW/GNU)")

# 3. Compile LuaJIT using its own build script / Makefile
if(WIN32)
  # msvcbuild.bat (no "static" arg) builds the DLL: lua51.dll + lua51.lib (import lib)
  set(LUAJIT_DLL      "${LUAJIT_SRC_DIR}/src/lua51.dll"
    CACHE FILEPATH "Path to the compiled LuaJIT DLL asset" FORCE)
    
  set(LUAJIT_IMPLIB   "${LUAJIT_SRC_DIR}/src/lua51.lib")

  ExternalProject_Add(luajit_compile
        SOURCE_DIR        "${LUAJIT_SRC_DIR}"
        BINARY_DIR        "${LUAJIT_SRC_DIR}/src"   # BUILD_COMMAND runs here
        CONFIGURE_COMMAND ""
        BUILD_COMMAND     cmd /C msvcbuild.bat       # .bat files need cmd /C on Windows
        INSTALL_COMMAND   ""
        BUILD_BYPRODUCTS  "${LUAJIT_DLL}"
                          "${LUAJIT_IMPLIB}"
    )

  add_library(luajit_lib SHARED IMPORTED GLOBAL)
  add_dependencies(luajit_lib luajit_compile)
  set_target_properties(luajit_lib PROPERTIES
        IMPORTED_LOCATION             "${LUAJIT_DLL}"
        IMPORTED_IMPLIB               "${LUAJIT_IMPLIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_SRC_DIR}/src"
    )

  # --- DLL copy ---
  # We want to copy lua51.dll next to the executable after it is built.
  # add_custom_command(TARGET start2d-noCPM POST_BUILD ...) is NOT allowed here
  # because that form is directory-scoped: the target must be defined in the
  # same CMakeLists.txt.  Instead we use two CMake features:
  #
  #   1. add_custom_target  — no directory restriction; runs the copy command.
  #      $<TARGET_FILE_DIR:start2d-noCPM> is a generator expression that is
  #      evaluated at generation time (after all CMakeLists.txt are processed),
  #      so it is fine to reference a target that doesn't exist yet.
  #
  #   2. cmake_language(DEFER DIRECTORY root CALL add_dependencies ...)
  #      Defers the add_dependencies call to the end of root CMakeLists.txt
  #      processing — by that point both start2d-noCPM and luajit_copy_dll
  #      are fully defined, so the call succeeds.

  # cmake_path(GET ${LUAJIT_DLL} FILENAME LUAJIT_DLL_FILENAME)
  # cmake_path(GET ${LUAJIT_DLL} FILENAME luadllfile)

  # Extract the filename with extension (main.cpp)
  cmake_path(GET LUAJIT_DLL FILENAME LUAJIT_DLL_FILENAME_WITH_EXT)

    # if(false)
    #   # add_custom_target(luajit_copy_dll
    #   #       COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #   #           "${LUAJIT_DLL}"
    #   #           "$<TARGET_FILE_DIR:start2d-noCPM>"
    #   #       DEPENDS luajit_compile
    #   #       COMMENT "Copying ${LUAJIT_DLL_FILENAME_WITH_EXT} to runtime executable directory..."
    #   #   )

    #   # message(STATUS "my current exe target is '${MY_EXE_TARGET}'")
    #   # message(STATUS "LUAJIT_DLL_FILENAME_WITH_EXT = '${LUAJIT_DLL_FILENAME_WITH_EXT}'")
    #   # message(STATUS "LUAJIT_DLL = '${LUAJIT_DLL}'")

    #   # cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}"
    #   #       #CALL add_dependencies start2d-noCPM luajit_copy_dll
    #   #       CALL add_dependencies ${MY_EXE_TARGET} luajit_copy_dll
    #   #   )




    #   # THIS WONT WORK
    #   # This error happens because add_custom_command(TARGET ...) can only be called in the exact same CMakeLists.txt file where add_executable(start2d-noCPM ...) is defined. CMake does not allow adding custom target commands from a sibling directory.Since you are in a sibling directory, you must stick with add_custom_target, but we need to fix the MSBuild directory creation bug.
    #   # # Trigger the copy IMMEDIATELY after ${MY_EXE_TARGET} finished building
    #   # add_custom_command(
    #   #     TARGET ${MY_EXE_TARGET} POST_BUILD
    #   #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #   #         "${LUAJIT_DLL}"
    #   #         "$<TARGET_FILE_DIR:${MY_EXE_TARGET}>"
    #   #     COMMENT "Copying ${LUAJIT_DLL_FILENAME_WITH_EXT} to runtime executable directory..."
    #   # )

    #   # # If luajit_compile is a custom target that must run BEFORE the exe builds,
    #   # # you just need this simple dependency rule:
    #   # add_dependencies(${MY_EXE_TARGET} luajit_compile)

    #   # Retrieve the path saved by the executable folder
    #   get_property(TARGET_DIR GLOBAL PROPERTY MAIN_EXE_BINARY_DIR)

    #   # Note: Visual Studio appends the configuration folder (Debug/Release),
    #   # so we use the built-in variable CMAKE_CFG_INTDIR to dynamically catch it.
    #   set(FINAL_DESTINATION "${TARGET_DIR}/${CMAKE_CFG_INTDIR}/${LUAJIT_DLL_FILENAME_WITH_EXT}")

    #   add_custom_target(luajit_copy_dll
    #       COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #           "${LUAJIT_DLL}"
    #           "${FINAL_DESTINATION}"
    #       DEPENDS luajit_compile
    #       COMMENT "Copying ${LUAJIT_DLL_FILENAME_WITH_EXT} to runtime directory..."
    #       VERBATIM
    #   )

    #   cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}"
    #       CALL add_dependencies ${MY_EXE_TARGET} luajit_copy_dll
    #   )
    # endif()

else()
  # Unix-based build using Make
  # The Makefile produces libluajit-5.1.a (not libluajit.a)
  set(LUAJIT_STATIC_LIB "${LUAJIT_SRC_DIR}/src/libluajit-5.1.a")

  # CMAKE_MAKE_PROGRAM may be ninja when using the Ninja generator,
  # so find the real make explicitly
  find_program(MAKE_PROGRAM NAMES make gmake REQUIRED)

  ExternalProject_Add(luajit_compile
        SOURCE_DIR        "${LUAJIT_SRC_DIR}"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND     ${MAKE_PROGRAM} -C "${LUAJIT_SRC_DIR}"
        INSTALL_COMMAND   ""
        BUILD_BYPRODUCTS  "${LUAJIT_STATIC_LIB}"
    )

  add_library(luajit_lib STATIC IMPORTED GLOBAL)
  add_dependencies(luajit_lib luajit_compile)
  set_target_properties(luajit_lib PROPERTIES
        IMPORTED_LOCATION             "${LUAJIT_STATIC_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_SRC_DIR}/src"
    )
  # No DLL copy needed on Unix — static lib is linked at build time.
endif()

add_library(lua_lib::lua_lib ALIAS luajit_lib)
