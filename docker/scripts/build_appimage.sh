#!/usr/bin/env bash
# Copyright (C) 2022 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

cd /opt/olive/core
mkdir build && \
cd build && \
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_COMPILER=clang++ && \
cmake --build . && \
ninja install && \
\
cd /opt/olive && \
mkdir build && \
cd build && \
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_COMPILER=clang++ && \
cmake --build . && \
\
cmake --install app --prefix appdir/usr && \
/usr/local/linuxdeployqt-x86_64.AppImage appdir/usr/share/applications/org.olivevideoeditor.Olive.desktop -appimage --appimage-extract-and-run
