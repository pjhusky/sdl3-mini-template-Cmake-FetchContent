set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Define the target name in one central place
set(MY_EXE_TARGET "start2d-noCPM" CACHE INTERNAL "Main executable target name")

option(PAWN_HOST_MEMORY "Use in-memory pawn host callbacks (no temp files on disk)" ON)
if(PAWN_HOST_MEMORY)
    set(_pawn_host_src pawn_host_mem.c)
else()
    set(_pawn_host_src pawn_host.c)
endif()
message(STATUS "Pawn host: ${_pawn_host_src} (PAWN_HOST_MEMORY=${PAWN_HOST_MEMORY})")

add_executable(${MY_EXE_TARGET}
    main.cpp
    ${_pawn_host_src}
)
unset(_pawn_host_src)

# Expose the option to C/C++ so mini_pawn.h can guard each sample variant
if(PAWN_HOST_MEMORY)
    target_compile_definitions(${MY_EXE_TARGET} PRIVATE PAWN_HOST_MEMORY)
endif()

target_compile_features(${MY_EXE_TARGET} PUBLIC cxx_std_17)

# option 1
# if(WIN32)
#     add_custom_command(TARGET ${MY_EXE_TARGET} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             $<TARGET_RUNTIME_DLLS:${MY_EXE_TARGET}>
#             $<TARGET_FILE_DIR:${MY_EXE_TARGET}>
#         COMMAND_EXPAND_LISTS
#     )
# endif()

# option 2
# if(WIN32)
#     add_custom_command(TARGET ${MY_EXE_TARGET} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             "${SDL3_BINARY_DIR}/SDL3.dll" # Adjust this path to your SDL3.dll location
#             $<TARGET_FILE_DIR:${MY_EXE_TARGET}>
#     )
# endif()

# 4. Ensure the external project builds BEFORE your executable
add_dependencies(${MY_EXE_TARGET} lua_lib::lua_lib)
# On Windows, LuaJIT is built via ExternalProject (luajit_compile) which generates
# luajit.h via its internal buildvm step; it is NOT a committed source file.
# add_dependencies(exe luajit_compile) only guards the LINK step in Ninja — the
# individual .cpp compile rules have no such ordering constraint and can race.
# OBJECT_DEPENDS on main.cpp creates a file-level Ninja edge:
#   main.cpp.obj → luajit.h (BYPRODUCT of luajit_compile)
# which forces luajit_compile to finish before the compile rule even starts.
FetchContent_GetProperties(luajit)   # populates luajit_SOURCE_DIR in local scope
if(WIN32 AND luajit_POPULATED)
    set_source_files_properties(main.cpp PROPERTIES
        OBJECT_DEPENDS "${luajit_SOURCE_DIR}/src/luajit.h")
endif()

### !!! CMAKE_LIBRARY_OUTPUT_DIRECTORY didn't help with copying SDL3.dll to the output directory, so we need to do it manually on Windows after build. !!! ###
if(WIN32)
  add_custom_command(TARGET ${MY_EXE_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${MY_EXE_TARGET}>
            $<TARGET_FILE_DIR:${MY_EXE_TARGET}>
        COMMAND_EXPAND_LISTS
    )
endif()

# If you are using Visual Studio, the path to the external project's DLL often changes based on the configuration (Debug, Release, etc.). You can make the path dynamic using the $<CONFIG> generator expression:
# set(EXTERNAL_DLL_PATH "${CMAKE_BINARY_DIR}/external/my_project/src/my_external_project-build/$<CONFIG>/external_lib.dll")


# Why lua51.dll was not picked up
# $<TARGET_RUNTIME_DLLS:${MY_EXE_TARGET}> only copies DLLs that CMake can infer as runtime dependencies of ${MY_EXE_TARGET}.

# In your case:

# SDL3::SDL3 is a normal imported/shared library target, so CMake can detect its runtime DLL and include it in TARGET_RUNTIME_DLLS.
# lua_lib::lua_lib is coming from your ExternalProject/Imported target setup, and CMake does not expose lua51.dll as a runtime DLL dependency for ${MY_EXE_TARGET} in that configuration.

# in SHORT:
# TARGET_RUNTIME_DLLS did not pick up lua51.dll because LuaJIT is not being exposed to CMake as a runtime dependency in the same way as SDL3. It is an imported external DLL that CMake cannot automatically collect for that generator expression in your setup.

# if(false)
#     # Get the filename from the path (make sure LUAJIT_DLL is a CACHE variable so this folder can see it)
#     # by adding "CACHE FILEPATH "Path to the compiled LuaJIT DLL asset" FORCE" to
#     # set(LUAJIT_DLL      "${LUAJIT_SRC_DIR}/src/lua51.dll") ==>
#     # set(LUAJIT_DLL      "${LUAJIT_SRC_DIR}/src/lua51.dll"
#     #    CACHE FILEPATH "Path to the compiled LuaJIT DLL asset" FORCE)
#     cmake_path(GET LUAJIT_DLL FILENAME LUAJIT_DLL_FILENAME_WITH_EXT)

#     # Clean, safe, local POST_BUILD command
#     add_custom_command(
#         TARGET ${MY_EXE_TARGET} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             "${LUAJIT_DLL}"
#             "$<TARGET_FILE_DIR:${MY_EXE_TARGET}>/${LUAJIT_DLL_FILENAME_WITH_EXT}"
#         COMMENT "Copying ${LUAJIT_DLL_FILENAME_WITH_EXT} to runtime executable directory..."
#         VERBATIM
#     )

#     # Tell the executable it must wait for luajit to compile first
#     add_dependencies(${MY_EXE_TARGET} luajit_compile)
# endif()


# configure_file(
#     ${CMAKE_SOURCE_DIR}/assets
#     ${CMAKE_BINARY_DIR}/bin/assets
#     COPYONLY
# )

#configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)
#configure_file(config.h.in config.h)
configure_file(config.h.in ${CMAKE_SOURCE_DIR}/src/config.h)

message(STATUS "Configured ImGui graphics backend in config.h: ${IMGUI_GFX_BACKEND}")
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

# Append the backend name to the final .exe name
set_target_properties(${MY_EXE_TARGET}
    PROPERTIES
    OUTPUT_NAME "${MY_EXE_TARGET}-${IMGUI_GFX_BACKEND}"
)

target_link_libraries(${MY_EXE_TARGET}
    PRIVATE

    imgui::imgui
    fmt::fmt

    SDL3::SDL3

    #################
    ### scripting ###
    #   !!! lua !!!
        lua_lib::lua_lib
        sol3::sol3

    #   !!! javascript !!!
        # quickjs::quickjs
        quickjs::quickjs-libc # ld: warning: ignoring duplicate libraries: '_deps/quickjs-build/libqjs.a'

    #   !!! gravity !!!
        gravity::gravity


    #   !!! pawn !!!
    dyncall::dyncall
    pawn::pawn
    ### scripting ###
    #################


    ###############
    ### physics ###
    box2d::box2d
    chipmunk::chipmunk
    ### physics ###
    ###############


)
# the 'quickjs::quickjs' above not only adds the lib, but also the include dirs
# however, 'quickjs::quickjs-libc' is needed as well, however, it re-adds the lib
# so include lib only once but add include dirs explicitly through 'quickjs::quickjs'
target_include_directories(${MY_EXE_TARGET} PRIVATE
    $<TARGET_PROPERTY:quickjs::quickjs,INTERFACE_INCLUDE_DIRECTORIES>
)

# pawn: amx.h (in amx/) includes sclinux.h (in compiler/) via a quoted include
# that falls back to the -I search path.  The INTERFACE dirs on pawn::pawn /
# pawn_bundle do not reliably propagate to the executable on all platforms, so
# add them explicitly here.  FetchContent_GetProperties gives us pawn_SOURCE_DIR
# because external/ is processed before src/ in the root CMakeLists.txt.
include(FetchContent)
FetchContent_GetProperties(pawn)
if(pawn_POPULATED)
    target_include_directories(${MY_EXE_TARGET} PRIVATE
        "${pawn_SOURCE_DIR}/compiler"
        "${pawn_SOURCE_DIR}/amx")
endif()


# Set .html suffix for web builds
if(EMSCRIPTEN)
    # This forces the final file to be index.html, index.js, and index.wasm
    set_target_properties(${MY_EXE_TARGET} PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")

    # 1. Add compiler options if needed (optional)
    target_compile_options(${MY_EXE_TARGET} PRIVATE
        -sUSE_SDL=3
        --use-port=emdawnwebgpu
    )

    target_link_options(${MY_EXE_TARGET} PRIVATE
        -sUSE_SDL=3
        --use-port=emdawnwebgpu
        # Optional: Tells Emscripten to generate an index.html wrapper instead of raw JS
        # --shell-file "${CMAKE_SOURCE_DIR}/libs/emscripten/shell_minimal.html"
    )

    # Optional: ensure it uses an HTML shell if you have one
    if(USE_EMSCRIPTEN_SHELL)
        target_link_options(${MY_EXE_TARGET} PRIVATE "--shell-file" "${CMAKE_SOURCE_DIR}/libs/emscripten/shell_minimal.html")
    endif()

    # $<TARGET_FILE_DIR:${MY_EXE_TARGET}> resolves to the directory where the final binary lands — which is build_web/bin because of the CMAKE_RUNTIME_OUTPUT_DIRECTORY set at the top of src/CMakeLists.txt. All 7 favicon files (favicon.ico, the PNGs, site.webmanifest) will be copied there when a change is detected for each build of the Emscripten target.
    # copy_directory_if_different does a per-file timestamp+size check and skips any file that's already identical in the destination — same semantics as copy_if_different but for a whole directory at once.
    add_custom_command(TARGET ${MY_EXE_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_SOURCE_DIR}/web-tools/favicon/ico"
            "$<TARGET_FILE_DIR:${MY_EXE_TARGET}>"
        COMMENT "Copying favicon assets to web output directory"
        VERBATIM
    )
    # else()
    #     copy_dlls(${APP})
endif()


# Define the runtime binary location (goes to 'bin/' inside your install folder)
install(TARGETS ${MY_EXE_TARGET}
    RUNTIME DESTINATION bin
)


# Automatically fetch and install the shared Chipmunk library (.dll / .so / .dylib) 
# ONLY if Chipmunk was built as a shared library
# if(TARGET chipmunk)
#     get_target_property(CHIPMUNK_TYPE chipmunk TYPE)
#     if(CHIPMUNK_TYPE STREQUAL "SHARED_LIBRARY")
#         install(TARGETS chipmunk
#             RUNTIME DESTINATION bin # DLLs on Windows go to bin/
#             LIBRARY DESTINATION lib # .so/.dylib on Unix go to lib/
#         )
#     endif()
# endif()