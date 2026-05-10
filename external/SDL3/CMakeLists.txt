include(FetchContent)

if(EMSCRIPTEN)
    # --- WEB CONFIGURATION ---
    # Use Emscripten's built-in ports for SDL3
    # Create a dummy INTERFACE target that includes the SDL3 compile/link flags
    if(NOT TARGET SDL3::SDL3)

        # NOTE: INTERFACE IMPORTED target needs the GLOBAL keyword to be visible in parent scopes
        add_library(SDL3::SDL3 INTERFACE IMPORTED GLOBAL)
        # Apply the Emscripten SDL3 flags to the interface target
        target_compile_options(SDL3::SDL3 INTERFACE "-sUSE_SDL=3")
        target_link_options(SDL3::SDL3 INTERFACE "-sUSE_SDL=3")
    endif()

else()
    # --- DESKTOP CONFIGURATION ---
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-3.4.8
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    FetchContent_MakeAvailable(SDL3)
endif()

