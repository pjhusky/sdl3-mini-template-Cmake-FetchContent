set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(start2d-noCPM
    main.cpp
)

# option 1
# if(WIN32)
#     add_custom_command(TARGET start2d-noCPM POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             $<TARGET_RUNTIME_DLLS:start2d-noCPM>
#             $<TARGET_FILE_DIR:start2d-noCPM>
#         COMMAND_EXPAND_LISTS
#     )
# endif()

# option 2
# if(WIN32)
#     add_custom_command(TARGET start2d-noCPM POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             "${SDL3_BINARY_DIR}/SDL3.dll" # Adjust this path to your SDL3.dll location
#             $<TARGET_FILE_DIR:start2d-noCPM>
#     )
# endif()


if(WIN32)
    add_custom_command(TARGET start2d-noCPM POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:start2d-noCPM>
            $<TARGET_FILE_DIR:start2d-noCPM>
        COMMAND_EXPAND_LISTS
    )
endif()

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
set_target_properties(start2d-noCPM 
    PROPERTIES 
    OUTPUT_NAME "start2d-noCPM-${IMGUI_GFX_BACKEND}"
)

target_link_libraries(start2d-noCPM
    PRIVATE
    imgui::imgui
    fmt::fmt
    SDL3::SDL3
)

# Set .html suffix for web builds
if(EMSCRIPTEN)
    # This forces the final file to be index.html, index.js, and index.wasm
    set_target_properties(start2d-noCPM PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")
    
    # Optional: ensure it uses an HTML shell if you have one
    target_link_options(start2d-noCPM PRIVATE "--shell-file" "${CMAKE_SOURCE_DIR}/libs/emscripten/shell_minimal.html")
# else()
#     copy_dlls(${APP})
endif()
