# Set architecture
ARCH=x86_64
MINGW_PATH=mingw64
NSISDEF=-DX64

# Update packages
pacman -Syu --noconfirm

# Install necessary packages
pacman -S --noconfirm --needed mingw-w64-$ARCH-toolchain mingw-w64-$ARCH-cmake mingw-w64-$ARCH-ninja mingw-w64-$ARCH-qt5 mingw-w64-$ARCH-ffmpeg mingw-w64-$ARCH-openimageio mingw-w64-$ARCH-opencolorio 

# Generate Ninja Makefiles
cmake . -G "Ninja" -DCMAKE_BUILD_TYPE=Debug

# Compile
ninja

# Get Git hash
GITHASH=$(git rev-parse --short=7 HEAD)

# Build finished, time to package
mkdir olive

# Copy main executable in
cd olive
cp ../app/olive-editor.exe .

# Inject Qt libraries
windeployqt --debug olive-editor.exe

# Inject all other libraries (loops over dependency output from ldd)
ldd olive-editor.exe | while read -r dep
do
	# Acquire full path to dependency
	dep_output=( $dep )
	dep_path=${dep_output[2]}

	# If this is a mingw dependency, copy it into the deploy folder
	if [[ $dep_path == /$MINGW_PATH* ]]
	then
		cp $dep_path .
	fi
done

# For some reason, this dep doesn't get listed by ldd but is necessary regardless
cp /$MINGW_PATH/bin/libcrypto*.dll .

# Create package name
PKGNAME=Olive-$GITHASH-Windows-$ARCH

# Package installer
cd ..
cp app/packaging/windows/nsis/* .
"/c/Program Files (x86)/NSIS/makensis.exe" -V4 $NSISDEF "-XOutFile $PKGNAME.exe" olive.nsi

# Package portable
touch olive/portable
7z a $PKGNAME.zip olive
