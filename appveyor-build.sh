# Update packages
pacman -Syu --noconfirm

# Install necessary packages
pacman -S --noconfirm mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-qt5 mingw-w64-x86_64-ffmpeg mingw-w64-x86_64-openimageio mingw-w64-x86_64-opencolorio 

# Generate Ninja Makefiles
cmake . -G "Ninja"

# Compile
ninja
