# TODO!!!

include(FetchContent)

# https://gitlab.kitware.com/cmake/cmake/-/work_items/22742
# GIT_TAG to reference a branch or tag
# GIT_TAG to reference a commit SHA

# In FetchContent_Declare, you specify a branch using the GIT_TAG argument.
# While the argument is named "TAG", it accepts any "commit-ish" reference, including branch names, tags, or specific commit hashes.
FetchContent_Declare(
    quickjs
    GIT_REPOSITORY git@github.com:bellard/quickjs.git
    GIT_TAG        04be246001599f5995fa2f2d8c91a0f198d3f34c
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable(quickjs)

add_library(quickjs)

target_include_directories(quickjs
    PUBLIC
    ${quickjs_SOURCE_DIR}
)

# basic version - works!
# target_sources(
#     imgui
#     PUBLIC
#     ${imgui_SOURCE_DIR}/imgui.cpp
#     ${imgui_SOURCE_DIR}/imgui_demo.cpp
#     ${imgui_SOURCE_DIR}/imgui_draw.cpp
#     ${imgui_SOURCE_DIR}/imgui_tables.cpp
#     ${imgui_SOURCE_DIR}/imgui_widgets.cpp
#     ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
#     # ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp # for SDL_Renderer backend
#     ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp # for SDL_GPU backend
# )

# Define the variable with a default value
# This line handles the variable setup and fallback value for your script logic.
#   What it does: It creates a variable named IMGUI_GFX_BACKEND in the cache.
#   The "OPENGL3" part: This specifies the default value used if the user completely forgets to pass a backend via the command line (e.g., if they just run cmake ..).
#   Why it's needed: Without this line, your CMake script would have no default value to work with, causing your if/else build logic to break.
#==> set(IMGUI_GFX_BACKEND "OPENGL3" CACHE STRING "Graphics backend for ImGui")
#
# This line handles the user interface constraints for tools like cmake-gui or ccmake.
#   What it does: It attaches metadata to the variable, telling CMake: "This variable behaves like an enum."
#   The list of backends: This populates the interactive drop-down menu in graphical CMake tools so users don't have to type out the backend names manually (and risk typos).
#   Why it's needed: Without this line, a user opening your project in cmake-gui would just see an empty text box where they could type anything, rather than a clean select menu.
#==> set_property(CACHE IMGUI_GFX_BACKEND PROPERTY STRINGS "OPENGL3" "SDL_RENDERER" "SDL_GPU" "SDL_VULKAN" )

# better:
# 1. Define the options list once
set(IMGUI_BACKEND_CHOICES "OPENGL3" "SDL_RENDERER" "SDL_GPU" "SDL_VULKAN")
# 2. Grab the first element to use as the default value ("OPENGL3")
list(GET IMGUI_BACKEND_CHOICES 0 IMGUI_DEFAULT_BACKEND)
# 3. Create the variable using the dynamic default
set(IMGUI_GFX_BACKEND "${IMGUI_DEFAULT_BACKEND}" CACHE STRING "Graphics backend for ImGui")
# 4. Attach the choices to the drop-down menu
set_property(CACHE IMGUI_GFX_BACKEND PROPERTY STRINGS ${IMGUI_BACKEND_CHOICES})

# Emscripten has no Vulkan and atm it seems it doesn't support SDL_GPU yet (SDL_GPU should map to WebGPU/WebGL once it's working...)
if(EMSCRIPTEN AND (IMGUI_GFX_BACKEND STREQUAL "SDL_VULKAN" OR IMGUI_GFX_BACKEND STREQUAL "SDL_GPU"))
    message(WARNING "Emscripten build: overriding ${IMGUI_GFX_BACKEND} -> OPENGL3")
    # FORCE updates the global cache so configure_file() in src/ sees OPENGL3 too.
    set(IMGUI_GFX_BACKEND "OPENGL3" CACHE STRING "Graphics backend for ImGui" FORCE)
    # Belt-and-suspenders: define the numeric value directly on imgui's consumers
    # so main.cpp sees the right IMGUI_GFX_BACKEND even if configure_file already
    # emitted config.h with the old cached value. The #ifndef in config.h.in
    # prevents a double-define. 1 == BACKEND_OPENGL3 per config.h.in.
    target_compile_definitions(imgui INTERFACE IMGUI_GFX_BACKEND=1)

    # How it works end-to-end:
    #
    # CACHE ... FORCE — external/imgui/ is processed before src/ in the build order, so this updates the global cache before configure_file() runs. config.h gets BACKEND_OPENGL3 from @IMGUI_GFX_BACKEND@. This is the primary fix.
    #
    # INTERFACE IMGUI_GFX_BACKEND=1 — propagates to main.cpp via target_link_libraries. If the cache update somehow ran after configure_file, the -D flag wins because compiler defines are processed before #include, and the #ifndef in config.h.in prevents the redefinition. This is the fallback.
    #
    # #ifndef IMGUI_GFX_BACKEND in config.h.in — lets the -D flag silently take precedence without a compiler warning about macro redefinition.
    #
    # INTERFACE (not PUBLIC) is used because only consumers of imgui (like main.cpp) need the override — the imgui library's own .cpp files don't reference IMGUI_GFX_BACKEND.

endif()

# Base source files
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
)

# Conditional backend selection
if(IMGUI_GFX_BACKEND STREQUAL "OPENGL3")
  list(APPEND IMGUI_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
  target_compile_definitions(imgui PUBLIC USE_IMGUI_OPENGL3)

elseif(IMGUI_GFX_BACKEND STREQUAL "SDL_RENDERER")
  list(APPEND IMGUI_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)
  target_compile_definitions(imgui PUBLIC USE_IMGUI_SDLRENDERER)

elseif(IMGUI_GFX_BACKEND STREQUAL "SDL_GPU")
  list(APPEND IMGUI_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp)
  target_compile_definitions(imgui PUBLIC USE_IMGUI_SDLGPU)

elseif(IMGUI_GFX_BACKEND STREQUAL "SDL_VULKAN")
  list(APPEND IMGUI_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
  target_compile_definitions(imgui PUBLIC USE_IMGUI_VULKAN)

else()
  # message(FATAL_ERROR "Invalid IMGUI_GFX_BACKEND value: ${IMGUI_GFX_BACKEND}. Valid options are: OPENGL3, SDL_RENDERER, SDL_GPU, SDL_VULKAN.")
  message(STATUS "Invalid IMGUI_GFX_BACKEND value: ${IMGUI_GFX_BACKEND}. Defaulting to SDL_GPU.")
  set(IMGUI_GFX_BACKEND "SDL_GPU" CACHE STRING "Graphics backend for ImGui" FORCE)
  list(APPEND IMGUI_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp)
  target_compile_definitions(imgui PUBLIC USE_IMGUI_SDLGPU)

endif()

message(STATUS "Selected ImGui graphics backend: ${IMGUI_GFX_BACKEND}")

# Apply to target
target_sources(imgui PRIVATE ${IMGUI_SOURCES})


target_link_libraries(imgui PUBLIC SDL3::SDL3)

if(IMGUI_GFX_BACKEND STREQUAL "OPENGL3")
  find_package(OpenGL REQUIRED)
  target_link_libraries(imgui PUBLIC OpenGL::GL)
  # target_link_libraries(imgui PUBLIC OpenGL::OpenGL)
elseif(IMGUI_GFX_BACKEND STREQUAL "SDL_VULKAN")
  find_package(Vulkan REQUIRED)
  target_link_libraries(imgui PUBLIC Vulkan::Vulkan)
endif()


add_library(imgui::imgui ALIAS imgui)
