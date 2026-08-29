# macOS Build Guide

This document describes how to build Oak Video Editor from source on macOS.

For the Chinese version, see [`build-macos-zh.md`](zh/build_macos-zh.md).

---

## Prerequisites

- macOS 12.0 (Monterey) or later
- [Homebrew](https://brew.sh/) package manager
- Xcode Command Line Tools

### Install Xcode Command Line Tools

```bash
xcode-select --install
```

---

## Install Dependencies

### 1. Install Homebrew (if not already installed)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 2. Install Build Tools and Libraries

```bash
brew update
brew install cmake ninja pkg-config
```

### 3. Install Qt 6

```bash
brew install qt@6
```

Add Qt 6 to your PATH (you may want to add this to your `~/.zshrc`):

```bash
echo 'export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### 4. Install FFmpeg

```bash
brew install ffmpeg
```

### 5. Install Image/Color Libraries

```bash
brew install openimageio opencolorio openexr
```

### 6. Install Audio and XML Libraries

```bash
brew install portaudio expat
```

### 7. Install Vulkan Backend Dependencies (Optional)

Only needed for the Vulkan render backend. Oak builds the OpenGL backend regardless and falls back to it at runtime if Vulkan is unavailable:

```bash
brew install molten-vk vulkan-headers vulkan-loader
```

Point CMake at the loader so `find_package(Vulkan)` succeeds (add to `~/.zshrc` if you build regularly):

```bash
export VULKAN_SDK="$(brew --prefix vulkan-loader)"
```

### 8. Install Test Framework (Optional)

Only needed if you plan to build and run tests:

```bash
brew install googletest
```

---

## Build OpenTimelineIO (Optional)

OpenTimelineIO enables importing/exporting timeline data in OTIO format. If you don't need OTIO support, you can skip this step.

```bash
# Clone the repository
git clone --depth 1 --branch v0.16.0 https://github.com/PixarAnimationStudios/OpenTimelineIO.git
cd OpenTimelineIO

# Configure and build
cmake -S . -B build -G Ninja \
  -DOTIO_SHARED_LIBS=ON \
  -DOTIO_PYTHON_BINDINGS=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${PWD}/install"

cmake --build build
cmake --install build
```

Note the installation path (e.g., `${PWD}/install`), you'll need it for the `OTIO_LOCATION` CMake option.

---

## Clone and Build Oak Video Editor

### 1. Clone the Repository

```bash
git clone --recursive https://github.com/OakVideoEditorCommunity/oak.git
cd oak
```

> **Note:** Make sure to use `--recursive` to clone submodules, as Oak depends on several external libraries included as submodules.

### 2. Configure with CMake

Basic configuration (without OTIO):

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DOCIO_LOCATION=$(brew --prefix opencolorio)
```

Configuration with OTIO support:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DOCIO_LOCATION=$(brew --prefix opencolorio) \
  -DOTIO_LOCATION=/path/to/otio/install \
  -DBUILD_TESTS=ON
```

### 3. Build

```bash
cmake --build build --config Release
```

The build process may take 10-30 minutes depending on your hardware.

---

## Run the Application

After successful build, you can run Oak Video Editor:

```bash
./build/app/oak-editor
```

Or open the app bundle (if generated):

```bash
open ./build/app/Oak.app
```

---

## Run Tests (Optional)

If you built with `-DBUILD_TESTS=ON`:

```bash
ctest --test-dir build --output-on-failure -C Release
```

---

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | `OFF` | Build unit tests |
| `BUILD_DOXYGEN` | `OFF` | Build Doxygen documentation |
| `USE_WERROR` | `OFF` | Treat warnings as errors |
| `OTIO_LOCATION` | - | Path to OpenTimelineIO installation (optional) |
| `OCIO_LOCATION` | - | Path to OpenColorIO installation |

---

## Troubleshooting

### Qt 6 Not Found

If CMake cannot find Qt 6, ensure it's in your PATH:

```bash
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6"
```

For Intel Macs, the path may be `/usr/local/opt/qt@6` instead.

### OpenColorIO Not Found

Make sure to specify the correct `OCIO_LOCATION`:

```bash
-DOCIO_LOCATION=$(brew --prefix opencolorio)
```

### OpenImageIO Not Found

Try reinstalling OpenImageIO:

```bash
brew reinstall openimageio
```

### PortAudio Issues

If you encounter audio-related build errors:

```bash
brew reinstall portaudio
export PKG_CONFIG_PATH="/opt/homebrew/opt/portaudio/lib/pkgconfig:$PKG_CONFIG_PATH"
```

### Apple Silicon (M1/M2/M3) Specific Issues

On Apple Silicon Macs, Homebrew installs to `/opt/homebrew` instead of `/usr/local`. Make sure your environment variables are set correctly:

```bash
export PATH="/opt/homebrew/bin:$PATH"
export LIBRARY_PATH="/opt/homebrew/lib:$LIBRARY_PATH"
export CPATH="/opt/homebrew/include:$CPATH"
```

---

## Creating an App Bundle

To create a distributable `.app` bundle, you may need to use `macdeployqt`:

```bash
/opt/homebrew/opt/qt@6/bin/macdeployqt build/app/Oak.app
```

This will bundle the required Qt libraries into the app.

---

## Uninstall

To remove the built application:

```bash
rm -rf build
```

To remove Homebrew dependencies (optional):

```bash
brew uninstall qt@6 ffmpeg openimageio opencolorio openexr portaudio expat googletest
```
