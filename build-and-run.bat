@ECHO OFF

IF NOT "%1"=="" (
    SET OPTION_BACKEND=%1
)

ECHO Available graphics backends: OPENGL3, SDL_RENDERER, SDL_GPU, SDL_VULKAN
ECHO Using backend: %OPTION_BACKEND%

@call create.bat %OPTION_BACKEND%
@call build.bat
@call run-release.bat %OPTION_BACKEND%

@ECHO ON
