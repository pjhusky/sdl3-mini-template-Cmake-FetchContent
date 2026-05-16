# as Chipmunk2D currently has a version setting of 3.7, we get build errors in more recent cmake versions:
# CMake Deprecation Warning at build/_deps/chipmunk-src/CMakeLists.txt:1 (cmake_minimum_required):
#   Compatibility with CMake < 3.10 will be removed from a future version of
#   CMake.
#
#   Update the VERSION argument <min> value.  Or, use the <min>...<max> syntax
#   to tell CMake that the project requires at least <min> but has been updated
#   to work with policies introduced by <max> or earlier.

# we can bump the version with this mechanism

cmake_policy(VERSION 3.10...3.31)