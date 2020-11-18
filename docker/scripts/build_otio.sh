#!/usr/bin/env bash
# Copyright (C) 2020 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

set -ex

git clone --depth 1 https://github.com/PixarAnimationStudios/OpenTimelineIO.git
cd OpenTimelineIO

#if [ "$OTIO_VERSION" != "latest" ]; then
#    git checkout "tags/v${OTIO_VERSION}" -b "v${OTIO_VERSION}"
#fi

pip install --prefix="${OLIVE_INSTALL_PREFIX}" .

mkdir build
cd build
cmake .. -G "Ninja"
cmake --build .
cmake --install . --prefix "${OLIVE_INSTALL_PREFIX}"

cd ../..
rm -rf OpenTimelineIO
