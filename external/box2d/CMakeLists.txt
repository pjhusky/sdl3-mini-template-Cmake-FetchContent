include(FetchContent)

FetchContent_Declare(
  box2d
  GIT_REPOSITORY https://github.com/erincatto/box2d.git
  GIT_TAG        v3.1.1
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)

# NOTE: this only affects the C/C++ preprocessor!
# add_compile_definitions(BUILD_SHARED_LIBS=ON)




# # Set the CMake variable to force shared libraries
# set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build shared libraries")

# if ( WiN32 )
#   # If you are compiling on Windows, you will likely also need to add set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON) right above your FetchContent_MakeAvailable call. Box2D uses standard C compilation which doesn't explicitly export all macro symbols via __declspec(dllexport) by default, meaning Windows needs this automatic fallback instruction to properly generate the required .lib import file alongside the .dll.
#   set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
# endif()

# FetchContent_MakeAvailable(box2d)





# Create a local variable scope block
block()
  # 1. Force shared library (only for this block)
  if(NOT EMSCRIPTEN)
    set(BUILD_SHARED_LIBS ON)
  endif()

  # 2. Export symbols on Windows (only for this block)
  # If you are compiling on Windows, you will likely also need to add set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON) right above your FetchContent_MakeAvailable call. Box2D uses standard C compilation which doesn't explicitly export all macro symbols via __declspec(dllexport) by default, meaning Windows needs this automatic fallback instruction to properly generate the required .lib import file alongside the .dll.
  if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  endif()

  # 3. Bring in the target. Box2D inherits the above two variables.
  FetchContent_MakeAvailable(box2d)
endblock()

# --- Outside the block, your main project remains untouched ---
# Any libraries declared down here will default to STATIC (unless overridden).


