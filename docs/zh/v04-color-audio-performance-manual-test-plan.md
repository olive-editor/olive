# v0.4 调色、音频与性能手工测试计划

本文档覆盖路线图里已合并到 v0.4 的原 0.4-0.6 范围：调色/LUT/示波器、音频同步/音频表、代理媒体、硬件加速导出、批量渲染队列。目标是用真实项目验证“可用、可保存、可重开、可导出、失败可回退”。

## 测试目标

- 验证常见创作流程从导入到导出不会崩溃或丢失项目状态。
- 验证调色、音频同步、代理媒体之间可以组合使用。
- 验证导出默认保留原片质量路径，不被代理媒体意外降级。
- 验证硬件依赖缺失时有安全回退或明确失败信息。
- 验证项目关闭重开后，LUT、调色、同步、代理和导出队列状态符合预期。

## 测试环境矩阵

至少覆盖以下环境中的两个；发布前尽量覆盖全部：

| 平台 | 必测项 | 备注 |
|:--|:--|:--|
| Linux + Mesa/AMD 或 Intel | 软件导出、代理、示波器、音频同步 | 当前开发主环境优先 |
| Linux + NVIDIA | NVENC、代理、4K/8K 预览 | 需要 NVIDIA 驱动和 ffmpeg 编码器支持 |
| Linux + Vulkan 驱动 | Vulkan 后端加载、viewer readback、代理、导出、OpenGL 回退 | 需先用 `vulkaninfo --summary` 确认可创建 Vulkan instance/device |
| macOS Apple Silicon | VideoToolbox、ColorSync/显示路径、代理 | 重点看硬件导出和 UI 响应 |
| Windows + NVIDIA/Intel | NVENC/QSV 可用性、路径编码、文件管理器 reveal | 重点看中文路径和空格路径 |
| Windows + Vulkan Runtime | Vulkan 后端加载、viewer readback、驱动缺失回退 | 重点看设置持久化、启动稳定性和显卡驱动兼容性 |

## 测试素材准备

准备一个专用测试目录，路径同时覆盖英文和中文，例如：`Oak v04 测试/素材 A`。

必备素材：

- `4k_camera_a.mov`：4K H.264/H.265，包含音频，时长 30 秒以上。
- `8k_or_heavy_camera.mov`：8K 或高码率 4K，用于代理压力测试。
- `dual_system_video.mov`：机内参考音频，画面有明显拍板或口型。
- `dual_system_audio.wav`：外录 WAV/BWF，含相同拍板或口型音频。
- `bwf_timecode.wav`：带 BWF 时间码元数据的 WAV。
- `noisy_dialogue.wav`：对白或音乐，用于 LUFS/VU 观察。
- `color_chart.mov`：含色卡、肤色、饱和色块和灰阶。
- `lut_valid.cube`、`lut_valid.3dl`：可明显改变画面的 LUT。
- `lut_invalid.cube`：故意损坏的 LUT，用于错误处理。
- `mixed_media_project`：至少 10 个 clip、不同分辨率、不同帧率、不同采样率。

## 通用通过标准

每个用例都按以下标准判断：

- 不崩溃，不出现 UI 死锁或无法取消的后台任务。
- 操作结果可在 viewer/timeline 中观察到。
- 保存项目、关闭应用、重新打开后，关键状态仍存在。
- 导出文件可播放，音画不同步不超过 1 帧，除非用例预期制造偏移。
- 错误输入不会污染项目状态；失败后原始媒体仍能播放。

## 0. 预检

1. 使用干净构建运行应用。
2. 打开新项目，设置项目 cache 到专用测试目录。
3. 导入所有测试素材。
4. 保存项目为 `v04-manual-test.oak`。
5. 关闭并重开项目，确认所有素材仍在线。

通过标准：项目重开后无素材丢失，无启动崩溃，cache 目录可写。

## 1. LUT 与调色测试

### 1.1 `.cube` LUT 导入和应用

1. 将 `color_chart.mov` 放入时间线。
2. 添加或打开 LUT/调色相关效果入口。
3. 加载 `lut_valid.cube`。
4. 播放 5 秒并拖动时间线。
5. 保存、关闭、重开项目。

通过标准：画面颜色明显变化；拖动时间线无明显卡死；重开后 LUT 仍生效。

### 1.2 `.3dl` LUT 导入和应用

1. 复制 1.1 流程，但加载 `lut_valid.3dl`。
2. 对比启用/禁用效果的画面差异。

通过标准：`.3dl` 生效；启用/禁用状态即时反映；无崩溃。

### 1.3 损坏 LUT 处理

1. 在同一 clip 上尝试加载 `lut_invalid.cube`。
2. 观察 UI 错误提示或效果状态。
3. 继续播放原 clip。
4. 保存并重开项目。

通过标准：损坏 LUT 不导致崩溃；原 clip 仍可播放；项目重开后不会卡在损坏状态。

### 1.4 三向色轮基础调整

1. 对 `color_chart.mov` 添加三向调色或打开三向色轮面板。
2. 分别调整 Shadows、Midtones、Highlights。
3. 调整强度到明显但不过曝。
4. 播放、暂停、逐帧查看。
5. 保存、重开项目。

通过标准：三个区域调整有可见差异；参数重开后保留；撤销/重做至少能回到前一状态。

### 1.5 调色与代理组合

1. 对重素材生成代理并启用代理。
2. 在启用代理状态下应用 LUT 和三向调色。
3. 关闭代理再观察同一帧。
4. 导出 5 秒测试片段。

通过标准：代理和原片路径颜色处理一致；导出默认使用原片质量路径，不因代理分辨率降低。

## 2. 示波器测试

### 2.1 Waveform

1. 打开 Scope 面板并选择 Waveform。
2. 播放 `color_chart.mov`。
3. 调整曝光/亮度相关参数。
4. 观察示波器变化。

通过标准：Waveform 随当前 viewer 帧和调色变化更新；无明显延迟堆积。

### 2.2 Vectorscope

1. 切换到 Vectorscope。
2. 使用含肤色和饱和色块的帧。
3. 调整饱和度或三向色轮。

通过标准：Vectorscope 分布随饱和度和色相变化；切换面板不崩溃。

### 2.3 Histogram

1. 切换到 Histogram。
2. 对灰阶或高反差画面调整亮度/对比度。
3. 播放和暂停不同帧。

通过标准：Histogram 更新正确；不会显示上一帧的长期残留。

### 2.4 Scope 面板压力

1. 连续在 Waveform、Vectorscope、Histogram 间切换 20 次。
2. 同时拖动时间线和调整调色参数。

通过标准：无 OpenGL/shader 崩溃；UI 不失去响应。

## 3. 波形自动同步测试

### 3.1 双系统音频自动同步

1. 将 `dual_system_video.mov` 和 `dual_system_audio.wav` 放入同一时间线。
2. 故意将外录音频偏移 1-3 秒。
3. 选择两个 clip，执行“按波形同步”。
4. 播放拍板或口型片段。

通过标准：同步后拍板峰值对齐；口型和声音误差不超过 1 帧；无误删 clip。

### 3.2 多 clip 同步

1. 放入 3 个视频 clip 和 3 条外录音频。
2. 每条音频设置不同初始偏移。
3. 一次选择所有相关 clip 执行波形同步。

通过标准：每组素材被合理对齐；无法匹配的素材保持原位或给出明确失败，不影响其他素材。

### 3.3 低质量参考音频

1. 使用噪声较大或音量较低的参考音频。
2. 执行波形同步。
3. 比较同步结果。

通过标准：同步失败时不能产生明显错误对齐且无提示；若无法匹配，应安全保留原位置或提示失败。

### 3.4 同步结果持久化

1. 完成同步后保存项目。
2. 关闭并重开。
3. 播放同步点。

通过标准：clip 位置保持；同步结果不随重开漂移。

## 4. BWF 时间码同步测试

### 4.1 读取 BWF 时间码

1. 导入 `bwf_timecode.wav`。
2. 查看素材属性或时间码相关入口。
3. 将其放入时间线并执行按时间码同步。

通过标准：能识别 BWF 起始时间码；同步操作不会把音频放到错误日期/极端时间位置。

### 4.2 BWF 与视频时间码对齐

1. 导入带匹配时间码的视频和 BWF 音频。
2. 执行按源时间码同步。
3. 播放同步点。

通过标准：音画对齐；时间线位置符合时间码差值。

### 4.3 缺失时间码回退

1. 选择没有 BWF 时间码的普通 WAV。
2. 执行按时间码同步。

通过标准：给出明确不可同步或安全跳过；不产生极端偏移。

## 5. 音频表测试

### 5.1 VU 表基础响应

1. 将 `noisy_dialogue.wav` 放入时间线。
2. 打开音频表面板。
3. 播放静音、对白、音乐段落。

通过标准：VU 表随音量实时变化；静音时回落；播放停止后状态合理。

### 5.2 LUFS 读数

1. 播放 30 秒对白或音乐。
2. 观察 LUFS 短时/综合读数入口。
3. 调整音量增益后重复播放。

通过标准：增益变化会反映到 LUFS；读数不会出现 NaN、无限大或明显跳变。

### 5.3 多声道音频

1. 导入立体声和多声道素材。
2. 播放并观察左右声道/总线表。
3. 静音或降低其中一个 clip 音量。

通过标准：声道显示符合素材声道；静音/增益调整即时反映。

## 6. 代理媒体测试

### 6.1 生成代理

1. 将 `8k_or_heavy_camera.mov` 放入时间线。
2. 右键 clip，执行 `Proxy > Generate Proxy`。
3. 观察任务列表和 cache/proxy 目录。
4. 等待任务完成。

通过标准：生成 `.working` 文件后最终变成 `.mp4`；任务完成后 `.working` 被清理；UI 不阻塞。

### 6.2 启用/禁用代理

1. 生成代理后勾选 `Proxy > Use Proxy`。
2. 播放时间线并观察流畅度。
3. 取消 `Use Proxy` 再播放。

通过标准：启用代理后预览可用；禁用后回到原片；缺失代理时安全回退原片。

### 6.3 Reveal 和 Delete Proxy

1. 执行 `Proxy > Reveal Proxy`。
2. 确认打开代理所在目录。
3. 执行 `Proxy > Delete Proxy`。
4. 再次播放。

通过标准：Reveal 指向 cache/proxy；Delete 删除代理文件和 working 文件；播放回退原片。

### 6.4 项目重开

1. 生成并启用代理后保存项目。
2. 关闭并重开。
3. 检查 `Use Proxy` 状态并播放。

通过标准：代理路径和启用状态保留；代理文件存在时继续可用。

### 6.5 代理失败路径

1. 临时移除或隐藏系统 `ffmpeg`，或使用不可读源文件。
2. 执行 Generate Proxy。
3. 观察任务失败后的项目状态。

通过标准：失败信息明确；`.working` 被清理；原始素材仍能播放；不会反复占用同一任务。

### 6.6 导出默认原片

1. 对 4K/8K clip 生成 720p 代理并启用。
2. 导出 4K 片段。
3. 检查导出分辨率和画质。

通过标准：导出不使用 720p 代理作为源；分辨率和细节符合原片路径。

## 7. 硬件加速导出测试

### 7.1 NVENC

1. 在 NVIDIA 环境打开导出设置。
2. 选择 H.264/H.265 NVENC 编码器。
3. 导出 30 秒 4K 片段。
4. 使用播放器或 ffprobe 检查输出。

通过标准：导出成功；输出编码格式正确；硬件编码不可用时有清晰错误或自动回退选项。

### 7.2 VideoToolbox

1. 在 macOS 打开导出设置。
2. 选择 VideoToolbox H.264/H.265。
3. 导出 30 秒 4K 片段。

通过标准：导出成功；系统负载符合硬件编码预期；输出可播放。

### 7.3 硬件导出失败回退

1. 选择当前机器不支持的硬件编码器。
2. 尝试导出。

通过标准：失败可理解，不生成损坏的完成文件；用户能改用软件编码继续导出。

## 8. 批量渲染队列测试

### 8.1 多任务队列

1. 创建 3 个 sequence：短片、含 LUT 片段、含代理片段。
2. 分别加入批量渲染队列。
3. 开始队列。

通过标准：任务按队列执行；每个输出文件独立生成；一个任务失败不应让整个应用崩溃。

### 8.2 队列取消

1. 加入一个较长导出任务。
2. 开始后立即取消。
3. 再加入短任务并执行。

通过标准：取消不会留下锁死状态；后续任务可继续运行；半成品文件有明确状态。

### 8.3 队列与项目保存

1. 配置多个队列任务。
2. 保存项目并重开。
3. 检查队列是否按当前设计保存或清空。

通过标准：行为必须明确且一致；如果队列不持久化，重开后应为空而不是半损坏状态。

## 9. 组合回归测试

### 9.1 完整剪辑链路

1. 创建 60 秒 sequence。
2. 混合 4K/8K、外录音频、LUT、三向调色、代理媒体。
3. 对部分 clip 做波形同步。
4. 打开 Scope 和音频表播放全片。
5. 导出软件编码版本。
6. 如果环境支持，再导出硬件编码版本。

通过标准：全流程无崩溃；导出文件音画同步；颜色和音量符合预览。

### 9.2 中文路径和空格路径

1. 将项目、素材、cache、导出目标放在包含中文和空格的路径。
2. 重复代理生成、LUT 加载、导出。

通过标准：路径处理正常；Reveal Proxy 和导出文件路径可打开。

### 9.3 长时间稳定性

1. 打开 4K/8K 项目循环播放 20 分钟。
2. 期间切换代理、Scope、音频表。
3. 观察内存和 UI 响应。

通过标准：内存没有持续不可控增长；播放停止后应用仍可操作和保存。

## 10. 图形后端与 Vulkan 手工测试

当前版本允许用户在 Preferences 中选择 OpenGL 或 Vulkan。动态后端适配器通过 `OAK_ENABLE_DYNAMIC_RENDER_BACKEND` 接入 `liboakgl` / `liboakvulkan`。Vulkan 后端已具备 offscreen texture、upload/download、shader Blit、viewer backend-neutral readback 的代码路径，但完整 UI 播放、代理、导出、Scope 和 OpenFX CPU 回退仍需要在真实显示环境与可用 Vulkan runtime 上手工验收。

Vulkan 测试必须先区分两类环境：

- 可用 Vulkan 环境：`vulkaninfo --summary` 能成功列出 instance、physical device、driver 和 queue family。
- 不可用 Vulkan 环境：缺少 runtime/ICD、驱动损坏，或 `vulkaninfo --summary` 报 `Found no drivers` / `ERROR_INCOMPATIBLE_DRIVER`。这类环境只测试回退，不应把 Vulkan 渲染用例记为通过。

### 10.1 默认 OpenGL 后端

1. 删除或备份现有用户配置。
2. 启动 Oak。
3. 打开 Preferences > Behavior > Rendering。
4. 检查 Graphics Backend 的默认值。
5. 导入 `color_chart.mov` 并播放。

通过标准：默认值为 OpenGL；viewer、Scope、调色和播放行为与原 OpenGL 路径一致。

### 10.2 Vulkan 运行时预检

1. 在待测机器上运行 `vulkaninfo --summary`。
2. 记录 GPU 型号、Vulkan API 版本、driver 版本、ICD 文件路径。
3. 确认应用构建产物中存在 Oak 私有 Vulkan 后端库：Linux 为 `liboakvulkan.so`，macOS 为 `liboakvulkan.dylib`，Windows 为 `oakvulkan.dll`。
4. 运行动态后端 gtest：`olive-gtest --gtest_filter='DynamicRenderBackend.*'`。
5. 检查 Vulkan 用例是实际执行还是 SKIP。

通过标准：可用 Vulkan 环境下 `vulkaninfo` 成功，`liboakvulkan` 存在，Vulkan gtest 至少执行 backend load、upload/download、Blit 相关用例；不可用 Vulkan 环境下测试必须明确记录 driver/runtime 错误，Vulkan gtest 可 SKIP，但 OpenGL fallback 用例必须通过。

### 10.3 切换到 Vulkan 并重启

1. 在 Preferences > Behavior > Rendering 中选择 `Vulkan (experimental)`。
2. 确认设置保存。
3. 关闭 Oak 并重新启动。
4. 再次打开 Preferences，确认仍显示 Vulkan。
5. 导入并播放 `4k_camera_a.mov`。
6. 检查日志，确认 `RenderManager` 报告的实际后端与回退结果一致。

通过标准：Vulkan 选择可持久化；重启后应用不崩溃；可用 Vulkan 环境下日志显示动态 Vulkan 后端加载并初始化成功，`RenderManager::backend()` 报告 Vulkan；不可用 Vulkan 环境下应明确回退 OpenGL，`RenderManager::backend()` 必须反映实际运行后端，播放仍可用。

### 10.4 Vulkan Viewer 基础播放

1. 在可用 Vulkan 环境中选择 Vulkan 并重启。
2. 导入 `color_chart.mov` 和 `4k_camera_a.mov`。
3. 将两个 clip 放入时间线，打开 Viewer。
4. 播放 10 秒，期间执行暂停、继续播放、逐帧前进、拖动时间线、缩放 Viewer。
5. 打开/关闭全屏或浮动 Viewer 窗口。
6. 观察画面是否黑屏、闪烁、残留上一帧或颜色明显错误。

通过标准：Viewer 通过 Vulkan backend-neutral readback 路径正常显示，播放和 seek 不崩溃；画面比例、裁切、缩放和 device pixel ratio 正常；没有长期黑屏、上一帧残留或 UI 死锁。

### 10.5 Vulkan H.265 4:2:2 4K 播放

1. 准备一段 `h265_422_4k.mov`，使用 `ffprobe` 确认视频流为 `hevc`，`pix_fmt` 为 `yuv422p10le` 或 `yuv422p12le`。
2. 选择 Vulkan 并重启。
3. 导入 `h265_422_4k.mov`，放入时间线并播放 10 秒。
4. 拖动时间线到多个位置，选择不同节点并重复刷新 Viewer。
5. 观察日志中是否出现 `Failed to allocate Vulkan staging buffer memory`。
6. 切换 OpenGL 后端重复同一素材播放，作为解码路径对照。

通过标准：Vulkan 下 Viewer 不黑屏、不闪烁且能稳定 seek；日志不应反复出现 Vulkan staging buffer 分配失败；若 Vulkan 环境确实内存不足，应给出明确失败或回退行为，不能持续显示一个非空但不可用的黑屏 texture。OpenGL 对照可播放时，Vulkan 失败应记录为 Vulkan 路径问题而不是素材不支持。

### 10.6 Vulkan 调色/LUT 显示一致性

1. 选择 Vulkan 并重启。
2. 将 `color_chart.mov` 放入时间线。
3. 加载 `lut_valid.cube`，再做一次三向色轮明显调整。
4. 在同一帧记录 Viewer 截图或视觉观察结果。
5. 切回 OpenGL 重启，打开同一项目并定位同一帧。
6. 对比 Vulkan 和 OpenGL 的画面颜色、亮度和透明度表现。

通过标准：Vulkan 与 OpenGL 预览颜色方向一致，LUT 和三向色轮均生效；不要求像素完全一致，但不能出现通道错乱、alpha 错误、明显 gamma 反转或 LUT 失效。

### 10.7 Vulkan 代理媒体与重素材播放

1. 选择 Vulkan 并重启。
2. 对 `8k_or_heavy_camera.mov` 生成代理并启用代理。
3. 播放代理路径 30 秒，期间拖动时间线和缩放 Viewer。
4. 关闭代理，播放原片路径 10 秒。
5. 保存、关闭并重开项目，确认代理状态仍正确。

通过标准：启用代理后 Viewer 可播放且不崩溃；禁用代理后回到原片路径；保存重开后代理状态一致；Vulkan 路径不应把导出源降级为代理。

### 10.8 Vulkan 软件导出

1. 选择 Vulkan 并重启。
2. 创建 10 秒 sequence，包含 `color_chart.mov`、LUT、三向调色、一个代理 clip 和一段音频。
3. 执行软件编码导出 H.264 或 ProRes。
4. 用播放器检查导出文件。
5. 使用 OpenGL 后端重复导出同一段作为对照。

通过标准：Vulkan 下导出成功，输出可播放，音画同步不超过 1 帧；颜色处理和 OpenGL 导出方向一致；启用代理时导出仍使用原片质量路径；失败时有明确错误，不生成损坏的完成文件。

### 10.9 Vulkan Scope 行为

1. 选择 Vulkan 并重启。
2. 打开 Waveform、Vectorscope、Histogram。
3. 播放 `color_chart.mov` 并调整 LUT/三向色轮。
4. 观察 Scope 面板行为。
5. 切回 OpenGL 后重复同一操作。

通过标准：当前 backend-neutral Scope 若仍是安全跳过，应明确记录为已知限制，且不能崩溃或卡死；OpenGL 下 Scope 必须正常更新。若 Vulkan Scope 已实现，则三类 Scope 必须随当前帧和调色变化更新。

### 10.10 Vulkan OpenFX CPU 回退

1. 选择 Vulkan 并重启。
2. 在 clip 上添加一个已知可用的 OFX 插件，优先选择支持 CPU 渲染且效果明显的插件。
3. 播放并导出 5 秒片段。
4. 检查日志中 OpenGL OFX render 是否被禁用，插件是否走 CPU readback/upload 路径。
5. 切回 OpenGL，确认支持 OpenGL render 的插件仍能走 OpenGL 输出绑定路径。

通过标准：Vulkan 下 OFX 插件不因缺少 OpenGL context 而被跳过或崩溃；CPU 回退输出可见且可导出；OpenGL 下原有 OFX OpenGL 路径不回退或失效。

### 10.11 Vulkan 后端长时间稳定性

1. 选择 Vulkan 并重启。
2. 打开包含 4K/8K、LUT、代理、音频和至少 10 个 clip 的项目。
3. 循环播放 20 分钟。
4. 期间反复 seek、切换代理、打开/关闭 Viewer、打开/关闭导出窗口。
5. 观察日志、显存/内存占用和 UI 响应。

通过标准：无崩溃、无持续不可控内存增长、无明显 Vulkan validation/driver error；停止播放后仍可保存项目和退出应用。

### 10.12 Vulkan 驱动缺失或不可用

1. 在没有 Vulkan Runtime 或驱动不可用的机器上选择 Vulkan。
2. 重启 Oak。
3. 打开项目并播放一段视频。
4. 检查日志或控制台输出。
5. 通过日志或调试接口确认 `RenderManager::backend()` 已同步为 OpenGL，而不是继续报告 Vulkan。

通过标准：应用可以启动；日志应说明 Vulkan 请求不可完全满足或当前回退 OpenGL；`RenderManager::backend()` 必须与实际运行后端一致；用户能回到 Preferences 改回 OpenGL。

### 10.13 从 Vulkan 切回 OpenGL

1. 在 Vulkan 已选中状态下打开 Preferences。
2. 将 Graphics Backend 改为 OpenGL。
3. 保存、退出并重启。
4. 播放同一项目并执行一次软件导出。

通过标准：重启后显示 OpenGL；播放和导出正常；不会保留错误的 Vulkan 状态。

### 10.14 代理、Scope 与调色组合回归

1. 选择 Vulkan 并重启。
2. 对 `8k_or_heavy_camera.mov` 生成并启用代理。
3. 添加 LUT 和三向调色。
4. 打开 Waveform、Vectorscope、Histogram 依次观察。
5. 切回 OpenGL 后重复同一段播放。

通过标准：Vulkan 请求状态下代理、Scope、调色不崩溃；切回 OpenGL 后项目状态一致；两种选择下导出默认仍使用原片。

### 10.15 动态 OpenGL 后端加载

1. 使用开启 `OAK_ENABLE_DYNAMIC_RENDER_BACKEND` 的实验构建。
2. 确认应用目录存在 Oak 私有 OpenGL 后端库，例如 `liboakgl.so`、`liboakgl.dylib` 或 `oakgl.dll`。
3. 在 Preferences 中选择 OpenGL 并重启。
4. 导入 `color_chart.mov`，播放、拖动时间线并打开 Scope。
5. 关闭应用，确认退出过程没有崩溃。

通过标准：日志显示动态 OpenGL 后端加载成功；viewer、Scope、调色和播放行为与默认 OpenGL 路径一致；退出时执行 destroy/unload 无崩溃。

### 10.16 动态后端缺失或损坏

1. 使用开启 `OAK_ENABLE_DYNAMIC_RENDER_BACKEND` 的实验构建。
2. 临时移走或重命名 Oak 私有 OpenGL 后端库。
3. 启动 Oak 并打开一个已有项目。
4. 观察日志、Preferences 和播放行为。

通过标准：应用不能静默崩溃；日志明确说明后端库加载失败；用户能够恢复库文件或切回默认构建继续打开项目。

### 10.17 Vulkan 动态后端库缺失或不可加载

1. 使用开启 `OAK_ENABLE_DYNAMIC_RENDER_BACKEND` 的实验构建。
2. 在 Preferences 中选择 Vulkan 并重启。
3. 临时移走、重命名或替换为损坏的 `liboakvulkan`。
4. 再次启动 Oak 并打开项目。
5. 观察回退行为。
6. 恢复 `liboakvulkan` 后切回 OpenGL 并重启。

通过标准：Vulkan 后端库缺失、损坏或符号不完整时不崩溃；日志明确说明 Vulkan 后端加载失败并回退或拒绝初始化；切回 OpenGL 后项目可播放。

### 10.18 Vulkan 与 OpenGL 结果记录

1. 对同一项目分别在 Vulkan 和 OpenGL 下执行 Viewer 播放、5 秒软件导出、代理启用导出。
2. 记录每个环境的实际 backend、GPU、driver、Vulkan API 版本和是否发生回退。
3. 对比导出文件的分辨率、帧率、时长、音频流和视觉结果。
4. 将差异记录到缺陷模板。

通过标准：每次测试结果能明确区分“真实 Vulkan 后端通过”、“请求 Vulkan 但回退 OpenGL 通过”和“Vulkan 后端失败”；不能把回退 OpenGL 的结果记为 Vulkan 渲染通过。

### 10.19 回退链路恢复

1. 在可用 Vulkan 环境中选择 Vulkan 并确认实际使用 Vulkan。
2. 退出应用，临时破坏 Vulkan runtime 或移走 `liboakvulkan`。
3. 启动应用并确认回退 OpenGL。
4. 切回 OpenGL 并重启。
5. 恢复 Vulkan runtime 和 `liboakvulkan`，再次选择 Vulkan 重启。

通过标准：回退和恢复路径都不破坏用户配置和项目文件；日志能解释每次实际使用的 backend；用户始终能回到可播放的 OpenGL 状态。

## 缺陷记录模板

每个失败项记录以下信息：

- 平台、GPU、驱动版本、FFmpeg 版本。
- Oak commit hash。
- 项目文件路径和素材类型。
- 复现步骤，精确到菜单项和参数。
- 预期结果和实际结果。
- 是否可稳定复现。
- 如果涉及导出，附 ffprobe 输出和导出设置截图。
- 如果涉及 Vulkan，附 `vulkaninfo --summary` 输出、实际 backend 日志、是否发生 OpenGL 回退。

## 发布前最低通过线

- 预检、LUT、三向色轮、三类 Scope、波形同步、BWF 时间码、音频表、代理生成/启用/删除、软件导出全部通过。
- 至少一个硬件编码环境通过 NVENC 或 VideoToolbox。
- OpenGL/Vulkan 图形后端选择、持久化、Vulkan 可用环境实渲染和 Vulkan 不可用环境回退测试通过；若无可用 Vulkan 环境，发布记录必须明确标注 Vulkan 实渲染未验收。
- 批量队列至少通过多任务执行和取消测试。
- 组合回归测试中的完整剪辑链路通过。
- 所有失败项有明确 issue 或文档化限制，不存在“无提示崩溃”级别问题。
