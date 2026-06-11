# cmake -S . -B ./build-macos-arm64 -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_BUILD_TYPE=Release -DIMGUI_GFX_BACKEND=SDL_VULKAN
cmake -S . -B ./build-macos-arm64 -G "Xcode" -DCMAKE_BUILD_TYPE=Release -DIMGUI_GFX_BACKEND=SDL_VULKAN
