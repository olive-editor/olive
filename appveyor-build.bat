REM Set up Visual C++ envs
call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86_amd64

REM Install FFmpeg
curl -L -o ffmpeg-4.2-win64-shared.zip https://ffmpeg.zeranoe.com/builds/win64/shared/ffmpeg-4.2-win64-shared.zip
curl -L -o ffmpeg-4.2-win64-dev.zip https://ffmpeg.zeranoe.com/builds/win64/dev/ffmpeg-4.2-win64-dev.zip
7z x ffmpeg-4.2-win64-shared.zip
7z x ffmpeg-4.2-win64-dev.zip

REM Install OpenColorIO
curl -L -o ocio-v1.1.1-win64-shared.zip https://github.com/olive-editor/OpenColorIO-Win32/releases/download/v1.1.1/ocio-v1.1.1-win64-shared.zip
7z x ocio-v1.1.1-win64-shared.zip

REM Install OpenImageIO
curl -L -o oiio-v2.3.0-win64-shared.zip https://github.com/olive-editor/OpenImageIO-Win32/releases/download/v2.3.0/oiio-v2.3.0-win64-shared.zip
7z x oiio-v2.3.0-win64-shared.zip

REM Run cmake
cmake . -G "NMake Makefiles" -DOPENCOLORIO_ROOT_DIR=ocio-v1.1.1-win64-shared -DFFMPEG_ROOT=ffmpeg-4.2-win64-dev -DOIIO_BASE_DIR=oiio-v2.3.0-win64-shared -DQt5_DIR=C:\Qt\5.13.0\msvc2015_64\lib\cmake\Qt5

REM Build Olive
nmake

REM For temporary testing, open remote connection
powershell $blockRdp = $true; iex ((new-object net.webclient).DownloadString('https://raw.githubusercontent.com/appveyor/ci/master/scripts/enable-rdp.ps1'))

REM Dependencies are installed, time to set up packages
mkdir olive
copy app\olive-editor.exe olive
cd olive
C:\Qt\5.13.0\msvc2015_64\bin\windeployqt olive-editor.exe
cd ..

REM Copy FFmpeg/OCIO/OIIO libraries in
copy ffmpeg-4.2-win64-shared\bin\*.dll olive\
copy ocio-v1.1.1-win64-shared\bin\*.dll olive\
copy oiio-v2.3.0-win64-shared\bin\*.dll olive\

REM Store the Git hash as a variable
git rev-parse --short=7 HEAD > shorthash.txt
set /p HASH= < shorthash.txt
git rev-parse HEAD > longhash.txt
set /p TRAVIS_COMMIT= < longhash.txt

REM First, we create the installer
copy app\packaging\windows\nsis\* .
"C:\Program Files (x86)\NSIS\makensis.exe" /V4 /DX64 "/XOutFile Olive-%HASH%-Win64.exe" olive.nsi

REM Next we create the portable version, which we'll need a "portable" file for
copy /b NUL olive\portable

REM Package the portable version into a zip
"C:\Program Files\7-Zip\7z.exe" a "Olive-%HASH%-Win64.zip" "olive"

REM We're done!
