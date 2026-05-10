@REM execute from web-tools directory
cmake -S ninja-bootstrap -B ninja-bootstrap/build
cmake --build ninja-bootstrap/build --config Release
cmake --install ninja-bootstrap/build --config Release --prefix ninja-bootstrap/install

@ECHO Ninja Generator Bootstrap built successfully. 

@REM ECHO Cleaning up build files...
@REM rmdir /S /Q ninja-bootstrap\build

