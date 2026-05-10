cmake_minimum_required(VERSION 3.14)
project(NinjaBootstrapper)

include(FetchContent)
FetchContent_Declare(
  ninja
  GIT_REPOSITORY https://github.com/ninja-build/ninja.git
  GIT_TAG        v1.13.2
)
# 2. Set the option BEFORE making it available
# Use CACHE ... FORCE to ensure it overrides any defaults in the subproject
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
# set_property(TARGET ninja PROPERTY BUILD_TESTING OFF) #?

FetchContent_MakeAvailable(ninja)



# include(../cmake/CPM.cmake)

# CPMAddPackage(
#   URI "gh:ninja-build/ninja#v1.13.2"
#   OPTIONS 
#    "BUILD_TESTING  OFF"
# )



