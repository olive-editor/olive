#!/usr/bin/env bash
# Copyright (C) 2022 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

set -ex

# TODO: Move to install_yumpackages.sh
yum install -y \
    libtiff \
    libtiff-devel \
    libpng \
    libpng-devel

git clone --depth 1 --branch "$JPEG_TURBO_VERSION" https://github.com/libjpeg-turbo/libjpeg-turbo
cd libjpeg-turbo
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX="${OLIVE_INSTALL_PREFIX}" \
      ..
make -j$(nproc)
make install
cd ../..
rm -rf libjpeg-turbo


# TODO: Make OIIO pick up WebP (missing: WEBPDEMUX_LIBRARY)
git clone --depth 1 --branch "$OIIO_VERSION" https://github.com/OpenImageIO/oiio.git
cd oiio

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX="${OLIVE_INSTALL_PREFIX}" \
      -DOIIO_BUILD_TOOLS=OFF \
      -DOIIO_BUILD_TESTS=OFF \
      -DVERBOSE=ON \
      -DUSE_PYTHON=0 \
      -DBoost_NO_BOOST_CMAKE=ON \
      ..
make -j$(nproc)
make install

cd ../..
rm -rf oiio
