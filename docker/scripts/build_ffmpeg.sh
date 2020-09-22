#!/usr/bin/env bash
# Copyright (C) 2020 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

# Largely copied from https://trac.ffmpeg.org/wiki/CompilationGuide/Centos
#
# Uses { command } & pattern for parallelism https://gist.github.com/thenadz/6c0584d42fb007582fbc
#
# TOOD: Use advanced options such as LTO? e.g. https://code.videolan.org/videolan/x264/-/blob/master/configure
# TODO: Enable debug symbols? (Or is it opt-out?)
# TODO: Add more ffmpeg libraries? See https://raw.githubusercontent.com/jrottenberg/ffmpeg/master/docker-images/4.2/centos7/Dockerfile

set -ex

# Set up recent NASM
{
    curl -fLsS -o nasm.tar.xz "https://www.nasm.us/pub/nasm/releasebuilds/${NASM_VERSION}/nasm-${NASM_VERSION}.tar.xz"
    tar xf nasm.tar.xz
    rm -f nasm.tar.xz
    cd nasm*
    ./autogen.sh
    ./configure \
      --prefix="${OLIVE_INSTALL_PREFIX}"
    make -j${NUM_JOBS}
    make install
    cd ..
    rm -rf nasm*
} &

# Set up recent YASM
{
    curl -fLsS -o yasm.tar.gz "http://www.tortall.net/projects/yasm/releases/yasm-${YASM_VERSION}.tar.gz"
    tar xf yasm.tar.gz
    rm -f yasm.tar.gz
    cd yasm*
    ./configure \
      --prefix="${OLIVE_INSTALL_PREFIX}"
    make -j${NUM_JOBS}
    make install
    cd ..
    rm -rf yasm*
} &

# join jobs, some libs depend on NASM/YASM
wait

# Set up libx264
{
    git clone --depth 1 "https://code.videolan.org/videolan/x264.git"
    cd x264
    ./configure \
      --prefix="${OLIVE_INSTALL_PREFIX}" \
      --enable-shared \
      --enable-pic \
      --disable-cli
    make -j${NUM_JOBS}
    make install
    cd ..
    rm -rf x264
} &

# Set up libx265
{
    # BitBucket dropped support for Mercurial repos
    #hg clone https://bitbucket.org/multicoreware/x265
    git clone --depth 1 "https://bitbucket.org/multicoreware/x265_git.git" x265
    cd x265/build/linux
    cmake \
      -G "Unix Makefiles" \
      -DCMAKE_INSTALL_PREFIX="${OLIVE_INSTALL_PREFIX}" \
      ../../source
    make -j${NUM_JOBS}
    make install
    cd ../../..
    rm -rf x265
} &

# Set up libmp3lame
{
    curl -fLsS -o lame.tar.gz "https://downloads.sourceforge.net/project/lame/lame/${LAME_VERSION}/lame-${LAME_VERSION}.tar.gz"
    tar xf lame.tar.gz
    rm -f lame.tar.gz
    cd lame*
    ./configure \
      --prefix="${OLIVE_INSTALL_PREFIX}" \
      --enable-shared \
      --enable-nasm \
      --disable-frontend
    make -j${NUM_JOBS}
    make install
    cd ..
    rm -rf lame*
} &

# Set up libopus
{
    curl -fLsS -o opus.tar.gz "https://archive.mozilla.org/pub/opus/opus-${OPUS_VERSION}.tar.gz"
    tar xf opus.tar.gz
    rm -f opus.tar.gz
    cd opus*
    ./configure \
      --prefix="${OLIVE_INSTALL_PREFIX}" \
      --enable-shared
    make -j${NUM_JOBS}
    make install
    cd ..
    rm -rf opus*
} &

# Set up libvpx
{
    git clone --depth 1 "https://chromium.googlesource.com/webm/libvpx.git"
    cd libvpx
    ./configure \
      --prefix="${OLIVE_INSTALL_PREFIX}" \
      --enable-shared \
      --enable-pic \
      --enable-vp9-highbitdepth \
      --as=yasm \
      --disable-examples \
      --disable-unit-tests \
      --disable-docs \
      --disable-install-bins
    make -j${NUM_JOBS}
    make install
    cd ..
    rm -rf libvpx
} &

# join all jobs
wait

curl -fLsS -o ffmpeg.tar.xz "https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.xz"
tar xf ffmpeg.tar.xz
cd ffmpeg*

# TODO: --enable-debug?
PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH" ./configure \
  --disable-doc \
  --disable-ffplay \
  --enable-gpl \
  --enable-version3 \
  --enable-shared \
  --enable-libfreetype \
  --enable-libmp3lame \
  --enable-libopus \
  --enable-libvpx \
  --enable-libx264 \
  --enable-libx265 \
  --prefix="${OLIVE_INSTALL_PREFIX}" \
  --extra-libs=-lpthread \
  --extra-libs=-lm \
  --extra-cflags="-I${OLIVE_INSTALL_PREFIX}/include" \
  --extra-ldflags="-L${OLIVE_INSTALL_PREFIX}/lib"
make -j${NUM_JOBS}
make install
cd ..
rm -rf ffmpeg*
