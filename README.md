# Oak Video Editor[![CI](https://github.com/OakVideoEditorCommunity/oak/actions/workflows/ci.yml/badge.svg)](https://github.com/OakVideoEditorCommunity/oak/actions/workflows/ci.yml)
 [中文](docs/zh/README.md)

Oak Video Editor is a free non-linear video editor for Windows, macOS, and Linux.

This project is a community-maintained fork of Olive Video Editor.
![screen](https://olivevideoeditor.org/img/020-2.png)


**NOTE: Oak Video Editor is alpha software and is considered highly unstable. While we highly appreciate users testing and providing usage information, please use at your own risk.**

## Binaries

The binary can be downloaded here:

[v0.4.0](https://github.com/OakVideoEditorCommunity/oak/releases/tag/v0.4.0-alpha)

## Building from Source

See [`docs/build.md`](docs/build.md) for build instructions on Windows (MSYS2), Linux (Debian/Ubuntu, Fedora, Arch Linux), and macOS.

## Roadmap

| Version | Theme | Core Deliverables | Boundary Notes |
|:--|:--|:--|:--|
| **0.3** (Current) | **Plugin Architecture Milestone** | Production-ready OpenFX host support | Not about quantity of plugins, but "any OFX plugin loads without crashing" |
| **0.4** | **Color, Audio & Performance** | `.cube`/`.3dl` support, scopes (waveform/vectorscope/histogram), three-way color wheels, waveform auto-sync, BWF timecode sync, audio meters (LUFS/VU), proxy media workflow, hardware-accelerated export (NVENC/VideoToolbox), batch render queue | Combines the previous 0.4-0.6 scope into one usability milestone: color workflow, audio sync, and 4K/8K performance |
| **0.5** | **Animation, Tracking & Collaboration** | Bézier keyframe curve editor, basic point tracking, image stabilizer, full multicam angle switching, OpenTimelineIO, EDL/XML import/export | Combines the previous 0.7-0.8 scope into one timeline/interchange milestone |
| **0.6** | **Stability Milestone** | Project file format freeze (backward compatibility promise), crash recovery, autosave, memory optimization | "Feature freeze" testing period before 1.0 |
| **1.0** | **Production Ready** | Complete documentation, installers, known issues list, community support channels | Declared "ready for serious projects" |
