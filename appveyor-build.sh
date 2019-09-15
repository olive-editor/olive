# Set env variable for the build architecture
ARCH=Win64

# Update packages
pacman -Syu --noconfirm

# Install necessary packages
pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-qt5 mingw-w64-x86_64-ffmpeg mingw-w64-x86_64-openimageio mingw-w64-x86_64-opencolorio 

# Generate Ninja Makefiles
cmake . -G "Ninja" -DCMAKE_BUILD_TYPE=Debug

# Compile
ninja

# Get Git hash
GITHASH=$(git rev-parse --short=7 HEAD)

# Create build name
BUILDNAME="Olive-$GITHASH-$ARCH"

# Build finished, time to package
mkdir olive

# Copy main executable in
cd olive
cp ../app/olive-editor.exe .

# Inject Qt libraries
windeployqt --debug olive-editor.exe

# Inject all other libraries
cp /mingw64/bin/libgcc_s_seh*.dll .
cp /mingw64/bin/libstdc++*.dll .
cp /mingw64/bin/av*.dll .
cp /mingw64/bin/sw*.dll .
cp /mingw64/bin/postproc*.dll .
cp /mingw64/bin/libOpenColorIO*.dll .
cp /mingw64/bin/libOpenImageIO*.dll .
cp /mingw64/bin/libwinpthread*.dll .
cp /mingw64/bin/libcelt*.dll .
cp /mingw64/bin/libgsm*.dll .

# Package installer
cd ..
cp app/packaging/windows/nsis/* .
"/c/Program Files (x86)/NSIS/makensis.exe" /V4 /DX64 "/XOutFile $BUILDNAME.exe" olive.nsi

# Package portable
touch olive/portable
7z a $BUILDNAME.zip olive