REM Set up Visual Studio x64 environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Install 64-bit
set VCPKG_DEFAULT_TRIPLET=x64-windows

REM Install Open*IO libraries
vcpkg install opencolorio
vcpkg install openimageio

REM Integrate libraries
cd c:\tools\vcpkg
vcpkg integrate install
cd %APPVEYOR_BUILD_FOLDER%

REM Acquire FFmpeg
set FFMPEG_VER=ffmpeg-4.2.1-win64
curl https://ffmpeg.zeranoe.com/builds/win64/dev/%FFMPEG_VER%-dev.zip > %FFMPEG_VER%-dev.zip
curl https://ffmpeg.zeranoe.com/builds/win64/shared/%FFMPEG_VER%-shared.zip > %FFMPEG_VER%-shared.zip
7z x %FFMPEG_VER%-dev.zip
7z x %FFMPEG_VER%-shared.zip

REM Add Qt and FFmpeg directory to path
set PATH=%PATH%;C:\Qt\5.13.2\msvc2017_64\bin;%APPVEYOR_BUILD_FOLDER%\%FFMPEG_VER%-dev

REM Run cmake
cmake -G "NMake Makefiles" . -DCMAKE_TOOLCHAIN_FILE=c:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake

REM Build with JOM
C:\Qt\Tools\QtCreator\bin\jom.exe

REM Check if this build should set up a debugging session
IF "%ENABLE_RDP%"=="1" (
    powershell -command  "$blockRdp = $true; iex ((new-object net.webclient).DownloadString('https://raw.githubusercontent.com/appveyor/ci/master/scripts/enable-rdp.ps1'))"
)
