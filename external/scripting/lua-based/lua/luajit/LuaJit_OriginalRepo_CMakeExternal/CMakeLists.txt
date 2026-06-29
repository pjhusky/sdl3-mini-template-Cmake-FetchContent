# cmake_language(DEFER) requires 3.19+
# cmake_path requires 3.20+
cmake_minimum_required(VERSION 3.20)


if(APPLE)
    execute_process(
        COMMAND sw_vers -productVersion
        OUTPUT_VARIABLE MACOS_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # Extract major.minor if needed, or use the full version string
    set(CMAKE_OSX_DEPLOYMENT_TARGET "${MACOS_VERSION}" CACHE STRING "" FORCE)

    message(STATUS " ### The macOS target version is: ${MACOS_VERSION} ### ")
    
endif()


project(MyLuaApp LANGUAGES C CXX)

# if(APPLE)
# target_compile_definitions(MyLuaApp PRIVATE MACOSX_DEPLOYMENT_TARGET="${MACOS_VERSION}")
# endif()

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
  # # Unix-based build using Make
  # # The Makefile produces libluajit-5.1.a (not libluajit.a)
  # set(LUAJIT_STATIC_LIB "${LUAJIT_SRC_DIR}/src/libluajit-5.1.a")

  # Check for the actual file produced by the Makefile
  # if(EXISTS "${LUAJIT_SRC_DIR}/src/libluajit.a")
  #   set(LUAJIT_STATIC_LIB "${LUAJIT_SRC_DIR}/src/libluajit.a")
  # else()
  #   set(LUAJIT_STATIC_LIB "${LUAJIT_SRC_DIR}/src/libluajit-5.1.a")
  # endif()
  set(LUAJIT_STATIC_LIB "${LUAJIT_SRC_DIR}/src/libluajit.a")

  message( STATUS " *#*#*# LUAJIT_STATIC_LIB = ${LUAJIT_STATIC_LIB} *#*#*# " )

  # CMAKE_MAKE_PROGRAM may be ninja when using the Ninja generator,
  # so find the real make explicitly
  find_program(MAKE_PROGRAM NAMES make gmake REQUIRED)

  # this worked, but relies on shell scripts passing several vars
  # ExternalProject_Add(luajit_compile
  #       SOURCE_DIR        "${LUAJIT_SRC_DIR}"
  #       CONFIGURE_COMMAND ""
  #       BUILD_COMMAND     ${MAKE_PROGRAM} -C "${LUAJIT_SRC_DIR}"
  #       INSTALL_COMMAND   ""
  #       BUILD_BYPRODUCTS  "${LUAJIT_STATIC_LIB}"
  # )


  # CMAKE_HOST_SYSTEM_PROCESSOR automatically holds the 'uname -m' equivalent
  message(STATUS " ---*** Native Architecture: ${CMAKE_HOST_SYSTEM_PROCESSOR} ***--- ")

  # 1. Define variables based on the active operating system
  if(APPLE)
      set(LUAJIT_ENV_VARS ${CMAKE_COMMAND} -E env MACOSX_DEPLOYMENT_TARGET=11.0)
      set(LUAJIT_EXTRA_FLAGS 
          "TARGET_CFLAGS=-arch \"${CMAKE_HOST_SYSTEM_PROCESSOR}\"" 
          "TARGET_LDFLAGS=-arch \"${CMAKE_HOST_SYSTEM_PROCESSOR}\""
      )
  else()
      # No deployment target needed on Linux, keep env clean
      set(LUAJIT_ENV_VARS "")
      # Standard x86_64 target flags are automatically handled by clang on Linux
      set(LUAJIT_EXTRA_FLAGS "")
  endif()

  # Passes Apple Silicon flags directly to LuaJIT's internal Makefile
  ExternalProject_Add(luajit_compile
      SOURCE_DIR        "${LUAJIT_SRC_DIR}"
      CONFIGURE_COMMAND ""
      # BUILD_COMMAND     ${CMAKE_COMMAND} -E env MACOSX_DEPLOYMENT_TARGET=11.0 
      #                   ${MAKE_PROGRAM} -C "${LUAJIT_SRC_DIR}" 
      #                   DEFAULT_CC=clang CROSS= TARGET_FLAGS="-arch arm64"
      # Safely separates Host CC compilation flags from Target architecture requirements
      # BUILD_COMMAND     ${CMAKE_COMMAND} -E env MACOSX_DEPLOYMENT_TARGET=11.0 
      #                   ${MAKE_PROGRAM} -C "${LUAJIT_SRC_DIR}" 
      #                   HOST_CC="clang"
      #                   TARGET_CC="clang"
      #                   TARGET_CFLAGS="-arch arm64"
      #                   TARGET_LDFLAGS="-arch arm64"


      # This error occurs because CMake expands quoted variables as a single argument containing a space, causing clang to read "-arch arm64" as one option instead of two (-arch and arm64).
      # To fix this, remove the inner quotes from the CMake arguments so that CMake passes them to the shell as distinct parameters.
      # ==> essential to have the 
      #     TARGET_CFLAGS="-arch arm64"
      #   changed to
      #     "TARGET_CFLAGS=-arch arm64"

      BUILD_COMMAND ${LUAJIT_ENV_VARS} 
                ${MAKE_PROGRAM} -C ${LUAJIT_SRC_DIR} 
                HOST_CC=clang TARGET_CC=clang 
                ${LUAJIT_EXTRA_FLAGS}

      # alternative: Use LuaJIT's Built-In Architecture Flags
      # BUILD_COMMAND ${CMAKE_COMMAND} -E env MACOSX_DEPLOYMENT_TARGET=11.0 
      #               ${MAKE_PROGRAM} -C ${LUAJIT_SRC_DIR} 
      #               HOST_CC=clang TARGET_CC=clang 
      #               TARGET_SYS=Darwin Am64=1

      
      INSTALL_COMMAND   ""
      BUILD_BYPRODUCTS  "${LUAJIT_STATIC_LIB}"
    )

  # 2. Dynamic Name Interception Step
  ExternalProject_Add_Step(luajit_compile detect_and_normalize_lib
    COMMAND           ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/detect_lib.cmake"
    DEPENDEES         build
    COMMENT           "Detecting output filename"
  )
  # 3. Generate the detection script that runs DURING the build phase
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/detect_lib.cmake" "
    file(GLOB DETECTED_LIBS \"${LUAJIT_SRC_DIR}/src/lib*.a\")
    
    if(DETECTED_LIBS)
        list(GET DETECTED_LIBS 0 ACTUAL_FILE)
        get_filename_component(RAW_NAME \"\${ACTUAL_FILE}\" NAME)
        
        # THIS WILL PRINT THE NAME DIRECTLY TO YOUR TERMINAL DURING THE BUILD
        message(NOTICE \"==================================================\")
        message(NOTICE \"[LuaJIT Detect] The Makefile output file is: \${RAW_NAME}\")
        message(NOTICE \"==================================================\")
        
        if(NOT \"\${ACTUAL_FILE}\" STREQUAL \"${LUAJIT_STATIC_LIB}\")
            file(COPY \"\${ACTUAL_FILE}\" DESTINATION \"${LUAJIT_SRC_DIR}/src/\")
            file(RENAME \"${LUAJIT_SRC_DIR}/src/\${RAW_NAME}\" \"${LUAJIT_STATIC_LIB}\")
        endif()
    else()
        message(FATAL_ERROR \"[LuaJIT Detect] Critical: The Makefile did not output any library file!\")
    endif()
  ")


  # copy_if_different: This is a built-in, cross-platform CMake tool. If libluajit.a and libluajit-5.1.a are identical (or if the Makefile somehow did create the -5.1.a file directly), it safely skips the copy operation without throwing errors.
  # DEPENDEES build: This guarantees that the copy operation will only run after the compilation step successfully completes.
  # Maintains compatibility: Your outer template can continue using ${LUAJIT_STATIC_LIB} exactly as it was originally written, meaning you don't have to trace down and refactor other parts of the template.

  # # 3. Safe Post-Build Copy Step
  # ExternalProject_Add_Step(luajit_compile safe_rename_copy
  #   COMMAND           ${CMAKE_COMMAND} -E copy_if_different
  #                     "${LUAJIT_SRC_DIR}/src/libluajit.a"
  #                     "${LUAJIT_STATIC_LIB}"
  #   DEPENDEES         build
  #   COMMENT           "Safely copying libluajit.a to libluajit-5.1.a if required"
  # )

  # # 3. Blind Wildcard Copy Step (Safety-First Execution)
  # ExternalProject_Add_Step(luajit_compile force_rename_output
  #   COMMAND           /bin/sh -c "
  #                     if [ ! -f '${LUAJIT_STATIC_LIB}' ]; then
  #                         # Look for any .a file produced in the target directory
  #                         FOUND_LIB=$(ls ${LUAJIT_SRC_DIR}/src/lib*.a 2>/dev/null | head -n 1);
  #                         if [ -n \"$FOUND_LIB\" ]; then
  #                             cp \"$FOUND_LIB\" \"${LUAJIT_STATIC_LIB}\";
  #                             echo 'Forced copy:' $FOUND_LIB '->' ${LUAJIT_STATIC_LIB};
  #                         fi
  #                     else
  #                         echo 'Target file already matched the template name. No action needed.';
  #                     fi
  #                     "
  #   DEPENDEES         build
  #   COMMENT           "Enforcing the given template filename from whatever the Makefile built"
  # )

  
  add_library(luajit_lib STATIC IMPORTED GLOBAL)
  add_dependencies(luajit_lib luajit_compile)
  set_target_properties(luajit_lib PROPERTIES
        IMPORTED_LOCATION             "${LUAJIT_STATIC_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_SRC_DIR}/src"
    )
  # No DLL copy needed on Unix — static lib is linked at build time.
endif()

add_library(lua_lib::lua_lib ALIAS luajit_lib)
