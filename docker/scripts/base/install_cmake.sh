#!/usr/bin/env bash
# Copyright (c) Contributors to the aswf-docker Project. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

set -ex

if [ ! -f "$DOWNLOADS_DIR/cmake-${CMAKE_VERSION}-Linux-x86_64.sh" ]; then
    curl --location "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.sh" -o "$DOWNLOADS_DIR/cmake-${CMAKE_VERSION}-Linux-x86_64.sh"
fi

sh "$DOWNLOADS_DIR/cmake-${CMAKE_VERSION}-Linux-x86_64.sh" --skip-license --prefix=/usr/local --exclude-subdir
