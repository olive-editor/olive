REM Set up MSYS environment
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%

REM Run MSYS script
bash -c ./appveyor-build.sh
