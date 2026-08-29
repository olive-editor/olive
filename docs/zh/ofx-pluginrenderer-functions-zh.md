# OFX PluginRenderer 函数说明（中文）

日期：2026-01-11
执行者：Codex

## 说明
本文档概述 `app/render/plugin/pluginrenderer.cpp` 与 `app/render/plugin/pluginrenderer.h` 中函数的职责，用于排查 OFX 渲染链路问题。

## 头文件（pluginrenderer.h）
- `olive::plugin::detail::BytesToPixels`：将字节行跨度转换为像素行跨度，供纹理读写使用。
- `olive::plugin::PluginRenderer`：OFX 插件渲染器，负责插件调用与 GL/CPU 纹理桥接。
- `PluginRenderer::AttachOutputTexture`：绑定输出纹理到 OFX 的 GL 输出路径。
- `PluginRenderer::DetachOutputTexture`：解除 OFX 的 GL 输出绑定。
- `PluginRenderer::RenderPlugin`：执行完整的 OFX 渲染流程（输入/输出准备、动作调用、结果处理）。

## 源文件（pluginrenderer.cpp）
- `GetOfxAVPixelFormat`：根据 OFX Image 的属性推导 FFmpeg 像素格式，并返回每像素字节数。
- `ApplyClipPreferencesToParams`：读取 clip 偏好（深度/组件）并更新 VideoParams。
- `PixelFormatFromOfxDepth`：OFX bit depth 字符串 → 内部 PixelFormat。
- `OfxDepthFromPixelFormat`：内部 PixelFormat → OFX bit depth 字符串。
- `ChannelCountFromOfxComponent`：OFX components 字符串 → 通道数。
- `OfxComponentsFromChannels`：通道数 → OFX components 字符串。
- `EffectSupportsPixelDepth`：检查插件是否支持指定像素深度。
- `ClipSupportsComponents`：检查 clip 是否支持指定组件格式。
- `ConversionCost`：估算源参数到目标参数的转换代价，用于排序。
- `ParamsConvertible`：判断目标参数能否映射为可用的 AVPixelFormat。
- `ConvertTextureForClip`：结合插件能力选择输入格式并执行转换。
- `create_avframe_from_ofx_image`：从 OFX Image 复制数据到 AVFrame（按图像属性推导格式）。
- `create_avframe_from_ofx_image_with_params`：按指定 VideoParams 复制 OFX Image 到 AVFrame。
- `GetDestinationAVPixelFormat`：将 VideoParams 映射为最终输出 AVPixelFormat。
- `GetRenderFieldForParams`：根据交错设置返回 OFX render field 字符串。
- `ReadbackTextureToFrame`：从 GPU 纹理回读到 AVFrame（必要时做格式转换）。
- `olive::plugin::detail::BytesToPixels`：字节行跨度 → 像素行跨度。
- `ConvertFrameIfNeeded`：必要时将 AVFrame 转换为目标 VideoParams 对应格式。
- `LinesizeToPixels`：字节行跨度 → 像素行跨度。
- `ConvertTextureForParams`：将纹理转换为指定 VideoParams（CPU 路径，必要时回读）。
- `PluginIdForInstance`：安全获取插件标识符，便于日志输出。
- `LogOfxFailure`：统一 OFX 调用失败日志输出。
- `LogClipState`：输出 clip 声明属性与 VideoParams，用于定位格式不一致。
- `LogImageProps`：输出 OFX Image 属性（深度/组件/行跨度/边界）。
- `MarkRenderFailure`：渲染失败时标记目标画面（紫色）。
- `PluginRenderer::RenderPlugin`：执行 OFX 插件渲染全流程。
- `PluginRenderer::AttachOutputTexture`：绑定输出纹理到 OFX GL 输出路径。
- `PluginRenderer::DetachOutputTexture`：解除 OFX GL 输出绑定。
