cmake_minimum_required(VERSION 3.21) # Note that TARGET_RUNTIME_DLLS requires CMake 3.21+ ... for copying SDL3.dll on Windows after build
project(start2d-noCPM)

add_subdirectory(src)
add_subdirectory(external)

