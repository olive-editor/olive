#!/usr/bin/env bash
# Copyright (C) 2020 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

set -ex

mkdir ocio
cd ocio

git clone --depth 1 https://github.com/AcademySoftwareFoundation/OpenColorIO.git
cd OpenColorIO

mkdir build
cd build
cmake \
    -DCMAKE_INSTALL_PREFIX="${OLIVE_INSTALL_PREFIX}" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DOCIO_BUILD_APPS=OFF \
    -DOCIO_BUILD_NUKE=OFF \
    -DOCIO_BUILD_DOCS=OFF \
    -DOCIO_BUILD_TESTS=OFF \
    -DOCIO_BUILD_GPU_TESTS=OFF \
    -DOCIO_USE_HEADLESS=OFF \
    -DOCIO_BUILD_PYTHON=OFF \
    -DOCIO_BUILD_JAVA=OFF \
    -DOCIO_WARNING_AS_ERROR=OFF \
    -DOCIO_INSTALL_EXT_PACKAGES=ALL \
    ..
make -j$(nproc)
make install

cd ../..

curl --location "https://github.com/imageworks/OpenColorIO-Configs/archive/v${OCIO_CONFIGS_VERSION}.tar.gz" -o "ocio-configs.tar.gz"
tar -zxf ocio-configs.tar.gz
cd "OpenColorIO-Configs-${OCIO_CONFIGS_VERSION}"

mkdir "${OLIVE_INSTALL_PREFIX}/openColorIO"
cp nuke-default/config.ocio "${OLIVE_INSTALL_PREFIX}/openColorIO/"
cp -r nuke-default/luts "${OLIVE_INSTALL_PREFIX}/openColorIO/"

cd ../..
rm -rf ocio
