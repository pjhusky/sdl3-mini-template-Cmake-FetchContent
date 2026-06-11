# no longer necessary, directly passed into CMake call
# export MACOSX_DEPLOYMENT_TARGET=11.0

cmake --build ./build-macos-arm64_Ninja --config Release -j
# -j
# -j 4
# --parallel 4
