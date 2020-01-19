REM Get git hash in variable [this seems to be the most efficient way]
git rev-parse --short=8 HEAD > hash.txt
set /p GITHASH= < hash.txt

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

REM Start building package
mkdir olive-editor
cd olive-editor
copy ..\app\olive-editor.exe .
windeployqt olive-editor.exe
copy ..\%FFMPEG_VER%-shared\bin\*.dll .
copy C:\Tools\vcpkg\packages\opencolorio_x64-windows\bin\OpenColorIO.dll .
copy C:\Tools\vcpkg\packages\openimageio_x64-windows\bin\OpenImageIO.dll .
copy C:\Tools\vcpkg\packages\yaml-cpp_x64-windows\bin\yaml-cpp.dll .
copy C:\Tools\vcpkg\packages\openexr_x64-windows\bin\*.dll .
copy C:\Tools\vcpkg\packages\libpng_x64-windows\bin\libpng16.dll .
copy C:\Tools\vcpkg\packages\libjpeg-turbo_x64-windows\bin\jpeg62.dll .
copy C:\Tools\vcpkg\packages\tiff_x64-windows\bin\tiff.dll .
copy C:\Tools\vcpkg\packages\zlib_x64-windows\bin\zlib1.dll .
copy C:\Tools\vcpkg\packages\liblzma_x64-windows\bin\lzma.dll .
copy C:\Tools\vcpkg\packages\boost-date-time_x64-windows\bin\boost_date_time-vc141-mt-x64-1_71.dll .
copy C:\Tools\vcpkg\packages\boost-filesystem_x64-windows\bin\boost_filesystem-vc141-mt-x64-1_71.dll .
copy C:\Tools\vcpkg\packages\boost-thread_x64-windows\bin\boost_thread-vc141-mt-x64-1_71.dll .

cd ..
set PKGNAME=Olive-%GITHASH%-Windows-x86_64

REM FIXME: Create installer

REM Create portable
copy nul olive-editor\portable
7z a %PKGNAME%.zip olive

REM Check if this build should set up a debugging session
IF "%ENABLE_RDP%"=="1" (
    powershell -command  "$blockRdp = $true; iex ((new-object net.webclient).DownloadString('https://raw.githubusercontent.com/appveyor/ci/master/scripts/enable-rdp.ps1'))"
)
