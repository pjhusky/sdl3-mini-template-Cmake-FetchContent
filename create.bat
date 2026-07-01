@ECHO OFF

@ECHO Available graphics backends: OPENGL3, SDL_RENDERER, SDL_GPU, SDL_VULKAN
@IF NOT "%~1"=="" (
    SET "OPTION_BACKEND=%~1"
)
@ECHO Using backend: "%OPTION_BACKEND%"


@REM Reset the variable so old runs don't corrupt this one
@SET CMAKE_DEFINES=
@REM This removes surrounding quotes from the second argument if they exist
@IF NOT "%~2"=="" (
    SET "CMAKE_DEFINES=%~2"
)
@ECHO CMAKE_DEFINES=%CMAKE_DEFINES%

@REM @ECHO.
@REM @ECHO ###############################
@REM @ECHO ### Start creating (%TIME%) ###
@REM @ECHO ###############################
@REM @ECHO.

@REM typically %2:
@REM -DLUA_MODE=Luajit_OriginalExternalAdd [Default]
@REM -DLUA_MODE=Luajit_WohlsoftCmakeWrapper
@REM -DLUA_MODE=VanillaLua

@REM -DPAWN_HOST_MEMORY=ON  ...   # in-memory (default)
@REM -DPAWN_HOST_MEMORY=OFF ...   # file-based (pawn_host.c)
@SET PAWN_MODE=-DPAWN_HOST_MEMORY=ON
@REM @SET PAWN_MODE=-DPAWN_HOST_MEMORY=OFF
cmake -S . -B .\build -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 18 2026" -A x64 -DIMGUI_GFX_BACKEND="%OPTION_BACKEND%" "%PAWN_MODE%" "%2" 
@REM %CMAKE_DEFINES%

@REM Use "--" to forward the argument to the underlying build tool (e.g., MSBuild or Ninja)
@REM cmake --build .\build --config Release -- -DLUA_MODE=Luajit_WohlsoftCmakeWrapper

@REM @ECHO.
@REM @ECHO ###############################
@REM @ECHO ### Finished building (%TIME%) ###
@REM @ECHO ###############################
@REM @ECHO.


@ECHO ON
