#include "config.h"

#if ( IMGUI_GFX_BACKEND == BACKEND_OPENGL3 )
    #include "main_ogl3.cpp"
#elif ( IMGUI_GFX_BACKEND == BACKEND_SDL_GPU )
    #include "main_sdlgpu.cpp"
#elif ( IMGUI_GFX_BACKEND == BACKEND_SDL_RENDERER )
    #include "main_sdlrenderer3.cpp"
#elif ( IMGUI_GFX_BACKEND == BACKEND_SDL_VULKAN )
    #include "main_sdlvulkan.cpp"
#else
    #error "Unsupported ImGui graphics backend"
#endif