#!/usr/bin/env bash
# Copyright (C) 2022 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

set -ex

git clone --depth 1 --branch "$OTIO_VERSION" https://github.com/PixarAnimationStudios/OpenTimelineIO.git
cd OpenTimelineIO

#pip install --prefix="${OLIVE_INSTALL_PREFIX}" .

mkdir build
cd build
cmake .. -G "Ninja" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX="${OLIVE_INSTALL_PREFIX}" \
  -DOTIO_PYTHON_INSTALL=OFF
cmake --build .
cmake --install .

cd ../..
rm -rf OpenTimelineIO
