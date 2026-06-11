@REM @SETLOCAL EnableDelayedExpansion  
@REM @SET START_TIME=%TIME%

@REM @ECHO.
@REM @ECHO ###############################
@REM @ECHO ### Start building (%START_TIME%) ###
@REM @ECHO ###############################
@REM @ECHO.

cmake --build .\build --config Release -j

@REM -j 4
@REM --parallel 4


@REM @SET END_TIME=%TIME%
@REM @ECHO.
@REM @ECHO ################################################################
@REM @ECHO ### Finished building (%END_TIME%) [Started at %START_TIME%] ###
@REM @ECHO ################################################################
@REM @ECHO.
