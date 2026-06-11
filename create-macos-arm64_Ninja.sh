# cmake -S . -B ./build-macos-arm64_Ninja -G "Ninja" -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_BUILD_TYPE=Release -DIMGUI_GFX_BACKEND=SDL_VULKAN
cmake -S . -B ./build-macos-arm64_Ninja -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DIMGUI_GFX_BACKEND=SDL_VULKAN
