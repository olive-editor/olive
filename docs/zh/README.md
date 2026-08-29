---
home: true
title: Oak 视频编辑器
heroImage: /images/oak-icon.png
heroText: Oak 视频编辑器
heroFullScreen: false
tagline: 现代开源的非线性剪辑器，强调速度与清晰度。
actions:
  - text: 阅读文档
    link: /zh/build.html
    type: primary
  - text: 查看工程文件
    link: /zh/project-file-reference.html
    type: secondary
features:
  - title: 快速剪辑
    details: 响应式时间线、智能缓存与高效媒体管理。
  - title: 面向创作者
    details: 简洁界面、可配置快捷键与清晰的工程结构。
  - title: 开源透明
    details: 公开开发流程，欢迎社区参与。
footer: Copyright © Oak Video Editor
---

## 关于 Oak

Oak 视频编辑器是 Olive 的重命名分支，目标是打造更完善、更友好的剪辑体验。本网站提供构建说明、工程文件参考与测试计划等贡献者文档。

## 快速开始

- 按《构建指南》在 Windows/macOS/Linux 上从源码构建。
- 在《工程文件参考》了解项目数据结构。
- 按《测试计划》确保发布质量。

## 下载

[v0.4.0](https://github.com/OakVideoEditorCommunity/oak/releases/tag/v0.4.0-alpha)


## 路线图

| 版本 | 主题 | 核心交付物 | 边界说明 |
|:--|:--|:--|:--|
| **0.3**（当前） | **插件架构里程碑** | OpenFX 宿主支持完整可用 | 不追求插件数量，追求"任意 OFX 插件加载不崩溃" |
| **0.4** | **调色、音频与性能** | `.cube`/`.3dl` 支持、示波器（波形/矢量/直方图）、三向色轮面板、波形自动同步（双系统录音对齐）、BWF 时间码同步、音频表（LUFS/VU）、代理媒体工作流、硬件加速导出（NVENC/VideoToolbox）、批量渲染队列 | 合并原 0.4-0.6 范围，集中解决调色工作流、音频同步和 4K/8K 可用性 |
| **0.5** | **动画、跟踪与协作** | 贝塞尔关键帧曲线编辑器、基础点跟踪、画面稳定器、完整 Multicam 角度切换、OpenTimelineIO、EDL/XML 导入导出 | 合并原 0.7-0.8 范围，集中处理时间线高级能力和外部工具交接 |
| **0.6** | **稳定性里程碑** | 项目文件格式冻结（向后兼容承诺）、崩溃恢复、Autosave、内存优化 | 1.0 前的"封版"测试期 |
| **1.0** | **生产就绪** | 文档完整、安装包、已知问题清单、社区支持渠道 | 宣告"可用于严肃项目" |
