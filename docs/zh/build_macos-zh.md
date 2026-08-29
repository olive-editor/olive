# macOS 编译指南

本文档介绍如何在 macOS 上从源代码构建 Oak 视频编辑器。

英文版本请参见 [`build-macos.md`](../build_macos.md)。

---

## 前置要求

- macOS 12.0 (Monterey) 或更高版本
- [Homebrew](https://brew.sh/) 包管理器
- Xcode 命令行工具

### 安装 Xcode 命令行工具

```bash
xcode-select --install
```

---

## 安装依赖

### 1. 安装 Homebrew（如果尚未安装）

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 2. 安装构建工具和库

```bash
brew update
brew install cmake ninja pkg-config
```

### 3. 安装 Qt 6

```bash
brew install qt@6
```

将 Qt 6 添加到你的 PATH（建议添加到 `~/.zshrc`）：

```bash
echo 'export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### 4. 安装 FFmpeg

```bash
brew install ffmpeg
```

### 5. 安装图像/色彩库

```bash
brew install openimageio opencolorio openexr
```

### 6. 安装音频和 XML 库

```bash
brew install portaudio expat
```

### 7. 安装 Vulkan 后端依赖（可选）

仅在需要 Vulkan 渲染后端时需要。Oak 始终构建 OpenGL 后端，当 Vulkan 不可用时会自动回退到它：

```bash
brew install molten-vk vulkan-headers vulkan-loader
```

让 CMake 能找到 loader，使 `find_package(Vulkan)` 成功（经常构建的话可以写入 `~/.zshrc`）：

```bash
export VULKAN_SDK="$(brew --prefix vulkan-loader)"
```

### 8. 安装测试框架（可选）

仅在需要构建和运行测试时需要：

```bash
brew install googletest
```

---

## 编译 OpenTimelineIO（可选）

OpenTimelineIO 支持以 OTIO 格式导入/导出时间线数据。如果你不需要 OTIO 支持，可以跳过此步骤。

```bash
# 克隆仓库
git clone --depth 1 --branch v0.16.0 https://github.com/PixarAnimationStudios/OpenTimelineIO.git
cd OpenTimelineIO

# 配置并编译
cmake -S . -B build -G Ninja \
  -DOTIO_SHARED_LIBS=ON \
  -DOTIO_PYTHON_BINDINGS=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${PWD}/install"

cmake --build build
cmake --install build
```

记下安装路径（例如 `${PWD}/install`），稍后在 CMake 配置中需要用到 `OTIO_LOCATION` 选项。

---

## 克隆并编译 Oak 视频编辑器

### 1. 克隆仓库

```bash
git clone --recursive https://github.com/OakVideoEditorCommunity/oak.git
cd oak
```

> **注意：** 请务必使用 `--recursive` 克隆子模块，因为 Oak 依赖于多个作为子模块包含的外部库。

### 2. 使用 CMake 配置

基础配置（不包含 OTIO）：

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DOCIO_LOCATION=$(brew --prefix opencolorio)
```

包含 OTIO 支持的配置：

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DOCIO_LOCATION=$(brew --prefix opencolorio) \
  -DOTIO_LOCATION=/path/to/otio/install \
  -DBUILD_TESTS=ON
```

### 3. 编译

```bash
cmake --build build --config Release
```

编译过程可能需要 10-30 分钟，具体取决于你的硬件配置。

---

## 运行应用程序

编译成功后，你可以运行 Oak 视频编辑器：

```bash
./build/app/oak-editor
```

或者打开应用程序包（如果已生成）：

```bash
open ./build/app/Oak.app
```

---

## 运行测试（可选）

如果你使用 `-DBUILD_TESTS=ON` 构建了项目：

```bash
ctest --test-dir build --output-on-failure -C Release
```

---

## 编译选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_TESTS` | `OFF` | 构建单元测试 |
| `BUILD_DOXYGEN` | `OFF` | 构建 Doxygen 文档 |
| `USE_WERROR` | `OFF` | 将警告视为错误 |
| `OTIO_LOCATION` | - | OpenTimelineIO 安装路径（可选） |
| `OCIO_LOCATION` | - | OpenColorIO 安装路径 |

---

## 故障排除

### 找不到 Qt 6

如果 CMake 无法找到 Qt 6，请确保它已在 PATH 中：

```bash
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6"
```

对于 Intel Mac，路径可能是 `/usr/local/opt/qt@6`。

### 找不到 OpenColorIO

确保指定了正确的 `OCIO_LOCATION`：

```bash
-DOCIO_LOCATION=$(brew --prefix opencolorio)
```

### 找不到 OpenImageIO

尝试重新安装 OpenImageIO：

```bash
brew reinstall openimageio
```

### PortAudio 问题

如果遇到与音频相关的编译错误：

```bash
brew reinstall portaudio
export PKG_CONFIG_PATH="/opt/homebrew/opt/portaudio/lib/pkgconfig:$PKG_CONFIG_PATH"
```

### Apple Silicon (M1/M2/M3) 特定问题

在 Apple Silicon Mac 上，Homebrew 安装到 `/opt/homebrew` 而不是 `/usr/local`。确保环境变量设置正确：

```bash
export PATH="/opt/homebrew/bin:$PATH"
export LIBRARY_PATH="/opt/homebrew/lib:$LIBRARY_PATH"
export CPATH="/opt/homebrew/include:$CPATH"
```

---

## 创建应用程序包

要创建可分发的 `.app` 包，你可能需要使用 `macdeployqt`：

```bash
/opt/homebrew/opt/qt@6/bin/macdeployqt build/app/Oak.app
```

这会将所需的 Qt 库打包到应用程序中。

---

## 卸载

要删除已构建的应用程序：

```bash
rm -rf build
```

要删除 Homebrew 依赖（可选）：

```bash
brew uninstall qt@6 ffmpeg openimageio opencolorio openexr portaudio expat googletest
```
