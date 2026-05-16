@REM @SETLOCAL EnableDelayedExpansion  

@ECHO OFF

IF NOT "%1"=="" (
    SET OPTION_BACKEND=%1
) ELSE (
    SET OPTION_BACKEND=SDL_GPU
)

@REM ECHO Available graphics backends: OPENGL3, SDL_RENDERER, SDL_GPU, SDL_VULKAN
@REM ECHO Using backend: %OPTION_BACKEND%

@SET START_TIME=%TIME%

@REM -DLUA_MODE=Luajit_OriginalExternalAdd [Default]
@REM -DLUA_MODE=Luajit_WohlsoftCmakeWrapper
@REM -DLUA_MODE=VanillaLua
@REM @SET "LUA_MODE=-DLUA_MODE=Luajit_OriginalExternalAdd"
@REM @SET "LUA_MODE=-DLUA_MODE=Luajit_WohlsoftCmakeWrapper"
@SET "LUA_MODE=-DLUA_MODE=VanillaLua"
@REM @call create.bat %OPTION_BACKEND% "-DLUA_MODE=Luajit_WohlsoftCmakeWrapper"
@call create.bat %OPTION_BACKEND% "%LUA_MODE%"
@call build.bat 

@SET END_TIME=%TIME%

@ECHO ### ***OVERALL*** Finished building (%END_TIME%) [Started at %START_TIME%] ###

@call run-release.bat %OPTION_BACKEND%

@ECHO ON
