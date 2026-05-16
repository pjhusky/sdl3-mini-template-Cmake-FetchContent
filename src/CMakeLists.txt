set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Define the target name in one central place
set(MY_EXE_TARGET "start2d-noCPM" CACHE INTERNAL "Main executable target name")

add_executable(${MY_EXE_TARGET}
    main.cpp
)

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
add_dependencies(${MY_EXE_TARGET} luajit_lib::luajit_lib)

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
# luajit_lib::luajit_lib is coming from your ExternalProject/Imported target setup, and CMake does not expose lua51.dll as a runtime DLL dependency for ${MY_EXE_TARGET} in that configuration.

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
    luajit_lib::luajit_lib
    sol3::sol3
    box2d::box2d
    chipmunk::chipmunk
)

# Set .html suffix for web builds
if(EMSCRIPTEN)
  # This forces the final file to be index.html, index.js, and index.wasm
  set_target_properties(${MY_EXE_TARGET} PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")

  # Optional: ensure it uses an HTML shell if you have one
  target_link_options(${MY_EXE_TARGET} PRIVATE "--shell-file" "${CMAKE_SOURCE_DIR}/libs/emscripten/shell_minimal.html")
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