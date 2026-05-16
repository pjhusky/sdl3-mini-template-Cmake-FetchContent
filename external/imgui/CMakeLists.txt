include(FetchContent)

# https://gitlab.kitware.com/cmake/cmake/-/work_items/22742
# GIT_TAG to reference a branch or tag
# GIT_TAG to reference a commit SHA

# In FetchContent_Declare, you specify a branch using the GIT_TAG argument.
# While the argument is named "TAG", it accepts any "commit-ish" reference, including branch names, tags, or specific commit hashes.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    #GIT_TAG        v1.92.7-docking
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable(imgui)

add_library(imgui)

target_include_directories(imgui
    PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
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
set(IMGUI_GFX_BACKEND "OPENGL3" CACHE STRING "Graphics backend for ImGui")
set_property(CACHE IMGUI_GFX_BACKEND PROPERTY STRINGS "OPENGL3" "SDL_RENDERER" "SDL_GPU" "SDL_VULKAN" )

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
