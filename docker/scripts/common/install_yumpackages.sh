#!/usr/bin/env bash
# Copyright (C) 2020 Olive Team
# Copyright (c) Contributors to the aswf-docker Project. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-or-later

set -ex

# Some packages are apparently in the powertools repo
sed -i -e 's/enabled=0/enabled=1/' /etc/yum.repos.d/CentOS-Linux-PowerTools.repo

# TODO: Check if this causes any problems. ASWF doesn't run an update.
dnf upgrade -y

# TODO: Add deps of deps which are explicitly listed in aswf-docker?
dnf install --setopt=tsflags=nodocs -y \
    bzip2-devel \
    cups-libs \
    freetype-devel \
    giflib-devel \
    git-core \
    gstreamer1 \
    gstreamer1-devel \
    gstreamer1-plugins-bad-free \
    gstreamer1-plugins-bad-free-devel \
    libicu-devel \
    libmng-devel \
    LibRaw-devel \
    libwebp-devel \
    libXcomposite \
    libXcomposite-devel \
    libXcursor \
    libXcursor-devel \
    libxkbcommon \
    libxkbcommon-devel \
    libxkbcommon-x11-devel \
    libXScrnSaver \
    libXScrnSaver-devel \
    mesa-libGL-devel \
    numactl-devel \
    openjpeg2-devel \
    pciutils-devel \
    pulseaudio-libs \
    pulseaudio-libs-devel \
    python3-tkinter \
    xcb-util-image \
    xcb-util-image-devel \
    xcb-util-keysyms \
    xcb-util-keysyms-devel \
    xcb-util-renderutil \
    xcb-util-renderutil-devel \
    xcb-util-wm \
    xcb-util-wm-devel \
    zlib-devel

# This is needed for Xvfb to function properly.
dbus-uuidgen > /etc/machine-id

#yum -y groupinstall "Development Tools"

# https://stackoverflow.com/a/61593093/2044940
dnf install -y gcc-toolset-9-gcc gcc-toolset-9-gcc-c++

# TODO: Does clearing the cache have any negative side effects?
dnf clean all

# HACK: Qt5GuiConfigExtras.cmake expects libGL.so in /usr/local/lib64 but it gets installed to /usr/lib64
ln -s /usr/lib64/libGL.so /usr/local/lib64/
# Alternatively, we could edit /usr/local/lib/cmake/Qt5Gui/Qt5GuiConfigExtras.cmake
# - _qt5gui_find_extra_libs(OPENGL "/usr/local/lib64/libGL.so" "" "")
# + _qt5gui_find_extra_libs(OPENGL "/usr/lib64/libGL.so" "" "")
