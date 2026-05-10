#pragma once

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #define EMSCRIPTEN_MAINLOOP_BEGIN emscripten_set_main_loop([]() {
    #define EMSCRIPTEN_MAINLOOP_END }, 0, 1);
#else
    #define EMSCRIPTEN_MAINLOOP_BEGIN
    #define EMSCRIPTEN_MAINLOOP_END
#endif
