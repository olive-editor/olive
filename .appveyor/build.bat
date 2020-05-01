REM Get git hash in variable [this seems to be the most efficient way]
git rev-parse --short=8 HEAD > hash.txt
git rev-parse HEAD > longhash.txt
set /p GITHASH= < hash.txt
set /p GITLONGHASH= < longhash.txt
set /p TRAVIS_COMMIT= < longhash.txt

REM Set up Visual Studio x64 environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Install 64-bit packages
set VCPKG_DEFAULT_TRIPLET=x64-windows

REM Hack to only install release builds for time
echo set(VCPKG_BUILD_TYPE release) >> C:\Tools\vcpkg\triplets\x64-windows.cmake

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
cmake -G "Ninja" . -DCMAKE_TOOLCHAIN_FILE=c:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo

REM Build with Ninja
ninja.exe || exit /B 1

REM If this is a pull request, no further packaging/deploying needs to be done
if "%APPVEYOR_PULL_REQUEST_NUMBER%" == "" goto end

REM Start building package
mkdir olive-editor
cd olive-editor
copy ..\app\olive-editor.exe .
copy ..\app\olive-editor.pdb .
copy ..\app\crashhandler.exe .
windeployqt olive-editor.exe
copy ..\%FFMPEG_VER%-shared\bin\*.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\OpenColorIO.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\OpenImageIO.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\yaml-cpp.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\Half-2_3.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\Iex-2_3.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\IexMath-2_3.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\IlmImf-2_3.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\IlmImfUtil-2_3.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\IlmThread-2_3.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\Imath-2_3.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\*.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\*.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\*.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\*.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\libpng16.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\jpeg62.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\tiff.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\zlib1.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\lzma.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\boost_date_time-vc141-mt-x64-1_72.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\boost_filesystem-vc141-mt-x64-1_72.dll .
copy C:\Tools\vcpkg\installed\x64-windows\bin\boost_thread-vc141-mt-x64-1_72.dll .

REM Package done, begin deployment
cd ..
set PKGNAME=Olive-%GITHASH%-Windows-x86_64

REM Create installer
copy app\packaging\windows\nsis\* .
"C:/Program Files (x86)/NSIS/makensis.exe" -V4 -DX64 "-XOutFile %PKGNAME%.exe" olive.nsi

REM Create portable
copy nul olive-editor\portable
7z a %PKGNAME%.zip olive-editor

REM We're ready to upload, but we only upload *sometimes*
REM set PATH=%PATH%;C:\msys64\usr\bin

REM If this was a tagged build, upload
if "%APPVEYOR_REPO_TAG%"=="true" GOTO upload

REM Else, if this is a continuous build, check if this commit is the most recent

REM Force locale to UTF-8 or grep -P fails
set LC_ALL=en_US.UTF-8

curl -H "Authorization: token %GITHUB_TOKEN%" https://api.github.com/repos/olive-editor/olive/commits/master > repoinfo.txt
grep -Po '(?^<=: \")(([a-z0-9])\w+)(?=\")' -m 1 repoinfo.txt > latestcommit.txt
set /p REMOTEHASH= < latestcommit.txt
if "%REMOTEHASH%"=="%GITLONGHASH%" GOTO upload

REM The previous if statements failed, skip to the end
GOTO end

:upload
set /p UPLOADTOOL_BODY= < latestcommit.txt

curl -L https://github.com/probonopd/uploadtool/raw/master/upload.sh > upload.sh
bash upload.sh Olive*.zip
bash upload.sh Olive*.exe

:end
REM Check if this build should set up a debugging session
IF "%ENABLE_RDP%"=="1" (
    powershell -command  "$blockRdp = $true; iex ((new-object net.webclient).DownloadString('https://raw.githubusercontent.com/appveyor/ci/master/scripts/enable-rdp.ps1'))"
)
