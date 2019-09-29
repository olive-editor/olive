REM Set up MSYS environment
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%

REM Run MSYS script
bash -c ./appveyor-build.sh

REM Check if this build should set up a debugging session
IF "%ENABLE_RDP%"=="1" (
    powershell -command  "$blockRdp = $true; iex ((new-object net.webclient).DownloadString('https://raw.githubusercontent.com/appveyor/ci/master/scripts/enable-rdp.ps1'))"
)
