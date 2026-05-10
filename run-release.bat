
@IF NOT "%1"=="" (
    @SET OPTION_BACKEND=%1
)

@pushd build\bin\Release
@start2d-noCPM-%OPTION_BACKEND%.exe
@popd