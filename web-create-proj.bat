@call c:\myDEV\emsdk\emsdk_env.bat

@REM NOTE: at least on windows, the emscripten SDK comes with a matching python version 
@REM we could just add it to our path:
@REM C:\myDEV\emsdk\python\3.13.3_64bit


@ECHO.
@ECHO Building (Ninja Generator Bootstrap)... 
@pushd web-tools
@call build-ninja.bat
@popd


@ECHO.
@ECHO Building (Main App)... 
@SET "LUA_MODE=-DLUA_MODE=VanillaLua"
@REM @SET "IMGUI_GFX_BACKEND=-DIMGUI_GFX_BACKEND=OPENGL3"
@REM @SET "IMGUI_GFX_BACKEND=-DIMGUI_GFX_BACKEND=SDL_RENDERER"
@REM @SET "IMGUI_GFX_BACKEND=-DIMGUI_GFX_BACKEND=SDL_GPU"
@SET "IMGUI_GFX_BACKEND=-DIMGUI_GFX_BACKEND=SDL_VULKAN"

@REM cmd /C starts terminal and runs the command, then closes the terminal when done. This was necessary as otherwise the following commands would not be run...
cmd /C emcmake cmake -G Ninja ^
  -DCMAKE_MAKE_PROGRAM=%CD%/web-tools/ninja-bootstrap/install/bin/ninja ^
  -S . -B .\build_web -DCMAKE_BUILD_TYPE=Release ^
  %LUA_MODE% ^
  %IMGUI_GFX_BACKEND%
  @REM -DIMGUI_GFX_BACKEND=SDL_RENDERER 
  @REM -DIMGUI_GFX_BACKEND=SDL_GPU
  @REM 
  @REM 
  
@REM emcmake cmake -G Ninja -B build_web -DCMAKE_MAKE_PROGRAM=/path/to/your/built/ninja

@REM "normal" cmake command is fine!
cmd /C cmake --build .\build_web


@ECHO.
@ECHO Starting minimal web server...
@REM python -m http.server --directory build_web/Release
@REM @pushd build_web
@REM @python -m http.server
@REM @popd

@python -m http.server -d build_web\bin