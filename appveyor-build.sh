# Set env variable for the build architecture
ARCH=Win64

# Update packages
pacman -Syu --noconfirm

# Install necessary packages
pacman -S --noconfirm mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-qt5 mingw-w64-x86_64-ffmpeg mingw-w64-x86_64-openimageio mingw-w64-x86_64-opencolorio 

# Generate Ninja Makefiles
cmake . -G "Ninja"

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
windeployqt olive-editor.exe

echo "FIXME: Doesn't inject FFmpeg/OCIO/OIIO libraries"

# Package installer
cd ..
cp app/packaging/windows/nsis/* .
"/c/Program Files (x86)/NSIS/makensis.exe" /V4 /DX64 "/XOutFile $BUILDNAME.exe" olive.nsi

# Package portable
touch olive/portable
7z a $BUILDNAME.zip olive