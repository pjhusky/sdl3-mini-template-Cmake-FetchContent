add_subdirectory(SDL3)
add_subdirectory(imgui) # because imgui depends on SDL3, it must be added after SDL3

add_subdirectory(fmt)