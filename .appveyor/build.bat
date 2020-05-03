REM Print all AppVeyor environment variables
echo APPVEYOR = "%APPVEYOR%" & REM True (true on Ubuntu image) if build runs in AppVeyor environment
echo CI = "%CI%" & REM True (true on Ubuntu image) if build runs in AppVeyor environment
echo APPVEYOR_URL = "%APPVEYOR_URL%" & REM AppVeyor central server(s) URL, https://ci.appveyor.com for Hosted service, specific server URL for on-premise
echo APPVEYOR_API_URL = "%APPVEYOR_API_URL%" & REM AppVeyor Build Agent API URL
echo APPVEYOR_ACCOUNT_NAME = "%APPVEYOR_ACCOUNT_NAME%" & REM account name
echo APPVEYOR_PROJECT_ID = "%APPVEYOR_PROJECT_ID%" & REM AppVeyor unique project ID
echo APPVEYOR_PROJECT_NAME = "%APPVEYOR_PROJECT_NAME%" & REM project name
echo APPVEYOR_PROJECT_SLUG = "%APPVEYOR_PROJECT_SLUG%" & REM project slug (as seen in project details URL)
echo APPVEYOR_BUILD_FOLDER = "%APPVEYOR_BUILD_FOLDER%" & REM path to clone directory
echo APPVEYOR_BUILD_ID = "%APPVEYOR_BUILD_ID%" & REM AppVeyor unique build ID
echo APPVEYOR_BUILD_NUMBER = "%APPVEYOR_BUILD_NUMBER%" & REM build number
echo APPVEYOR_BUILD_VERSION = "%APPVEYOR_BUILD_VERSION%" & REM build version
echo APPVEYOR_BUILD_WORKER_IMAGE = "%APPVEYOR_BUILD_WORKER_IMAGE%" & REM current build worker image the build is running on, e.g. Visual Studio 2015 (also can be used as “tweak” environment variable to set build worker image)
echo APPVEYOR_PULL_REQUEST_NUMBER = "%APPVEYOR_PULL_REQUEST_NUMBER%" & REM Pull (Merge) Request number
echo APPVEYOR_PULL_REQUEST_TITLE = "%APPVEYOR_PULL_REQUEST_TITLE%" & REM Pull (Merge) Request title
echo APPVEYOR_PULL_REQUEST_HEAD_REPO_NAME = "%APPVEYOR_PULL_REQUEST_HEAD_REPO_NAME%" & REM Pull (Merge) Request source repo
echo APPVEYOR_PULL_REQUEST_HEAD_REPO_BRANCH = "%APPVEYOR_PULL_REQUEST_HEAD_REPO_BRANCH%" & REM Pull (Merge) Request source branch
echo APPVEYOR_PULL_REQUEST_HEAD_COMMIT = "%APPVEYOR_PULL_REQUEST_HEAD_COMMIT%" & REM Pull (Merge) Request source commit ID (SHA)
echo APPVEYOR_JOB_ID = "%APPVEYOR_JOB_ID%" & REM AppVeyor unique job ID
echo APPVEYOR_JOB_NAME = "%APPVEYOR_JOB_NAME%" & REM job name
echo APPVEYOR_JOB_NUMBER = "%APPVEYOR_JOB_NUMBER%" & REM job number, i.g. 1, 2, etc.
echo APPVEYOR_REPO_PROVIDER = "%APPVEYOR_REPO_PROVIDER%" & REM gitHub, bitBucket, kiln, vso, gitLab, gitHubEnterprise, gitLabEnterprise, stash, gitea, git, mercurial or subversion
echo APPVEYOR_REPO_SCM = "%APPVEYOR_REPO_SCM%" & REM git or mercurial
echo APPVEYOR_REPO_NAME = "%APPVEYOR_REPO_NAME%" & REM repository name in format owner-name/repo-name
echo APPVEYOR_REPO_BRANCH = "%APPVEYOR_REPO_BRANCH%" & REM build branch. For Pull Request commits it is base branch PR is merging into
echo APPVEYOR_REPO_TAG = "%APPVEYOR_REPO_TAG%" & REM true if build has started by pushed tag; otherwise false
echo APPVEYOR_REPO_TAG_NAME = "%APPVEYOR_REPO_TAG_NAME%" & REM contains tag name for builds started by tag; otherwise this variable is undefined
echo APPVEYOR_REPO_COMMIT = "%APPVEYOR_REPO_COMMIT%" & REM commit ID (SHA)
echo APPVEYOR_REPO_COMMIT_AUTHOR = "%APPVEYOR_REPO_COMMIT_AUTHOR%" & REM commit author’s name
echo APPVEYOR_REPO_COMMIT_AUTHOR_EMAIL = "%APPVEYOR_REPO_COMMIT_AUTHOR_EMAIL%" & REM commit author’s email address
echo APPVEYOR_REPO_COMMIT_TIMESTAMP = "%APPVEYOR_REPO_COMMIT_TIMESTAMP%" & REM commit date/time in ISO 8601 format
echo APPVEYOR_REPO_COMMIT_MESSAGE = "%APPVEYOR_REPO_COMMIT_MESSAGE%" & REM commit message
echo APPVEYOR_REPO_COMMIT_MESSAGE_EXTENDED = "%APPVEYOR_REPO_COMMIT_MESSAGE_EXTENDED%" & REM the rest of commit message after line break (if exists)
echo APPVEYOR_SCHEDULED_BUILD = "%APPVEYOR_SCHEDULED_BUILD%" & REM True if the build runs by scheduler
echo APPVEYOR_FORCED_BUILD (True or undefined) = "%APPVEYOR_FORCED_BUILD (True or undefined)%" & REM builds started by “New build” button or from the same API
echo APPVEYOR_RE_BUILD (True or undefined) = "%APPVEYOR_RE_BUILD (True or undefined)%" & REM build started by “Re-build commit/PR” button of from the same API
echo APPVEYOR_RE_RUN_INCOMPLETE (True or undefined) = "%APPVEYOR_RE_RUN_INCOMPLETE (True or undefined)%" & REM build job started by “Re-run incomplete” button of from the same API
echo PLATFORM = "%PLATFORM%" & REM platform name set on Build tab of project settings (or through platform parameter in appveyor.yml)
echo CONFIGURATION = "%CONFIGURATION%" & REM configuration name set on Build tab of project settings (or through configuration parameter in appveyor.yml)

REM Don't do a full deployment if this is a pull request
if NOT "%APPVEYOR_PULL_REQUEST_NUMBER%" == "" (
    set SKIP_INSTALLER=true
    set SKIP_UPLOAD=true
)

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
if "%SKIP_INSTALLER%"=="true" goto portable
copy app\packaging\windows\nsis\* .
"C:/Program Files (x86)/NSIS/makensis.exe" -V4 -DX64 "-XOutFile %PKGNAME%.exe" olive.nsi
)

:portable
REM Create portable
copy nul olive-editor\portable
7z a %PKGNAME%.zip olive-editor

REM We're ready to upload, but we only upload *sometimes*
if "%SKIP_UPLOAD%"=="true" goto end

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
