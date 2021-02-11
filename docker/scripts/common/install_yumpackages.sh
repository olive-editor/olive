#!/usr/bin/env bash
# Copyright (C) 2020 Olive Team
# Copyright (c) Contributors to the aswf-docker Project. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-or-later

set -ex

# TODO: Check if this causes any problems. ASWF doesn't run a yum update.
yum update -y

# TODO: Add deps of deps which are explicitly listed in aswf-docker?
yum install --setopt=tsflags=nodocs -y \
    bzip2-devel \
    cups-libs \
    freetype-devel \
    giflib-devel \
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
    zlib-devel

# This is needed for Xvfb to function properly.
dbus-uuidgen > /etc/machine-id

yum -y groupinstall "Development Tools"

# TODO: Below code installs the obsolete devtoolset-6.
#       Unclear which devtoolset it will be for VFX platform CY2021:
#       https://groups.google.com/forum/#!topic/vfx-platform-discuss/_-_CPw1fD3c

yum install -y --setopt=tsflags=nodocs centos-release-scl-rh yum-utils

if [[ $DTS_VERSION == 6 ]]; then
    # Use the centos vault as the original devtoolset-6 is not part of CentOS-7 anymore
    sed -i 's/7/7.6.1810/g; s|^#\s*\(baseurl=http://\)mirror|\1vault|g; /mirrorlist/d' /etc/yum.repos.d/CentOS-SCLo-*.repo
fi

yum install -y --setopt=tsflags=nodocs \
    "devtoolset-$DTS_VERSION-toolchain"

yum install -y epel-release

# Additional package that are not found initially
yum install -y \
    rh-git218
#   lame-devel
#   libcaca-devel \
#   libdb4-devel \
#   libdc1394-devel \
#   openssl11-devel \
#   p7zip \
#   yasm-devel \
#   zvbi-devel

# TODO: Does clearing the cache have any negative side effects?
yum clean all

# HACK: Qt5GuiConfigExtras.cmake expects libGL.so in /usr/local/lib64 but it gets installed to /usr/lib64
ln -s /usr/lib64/libGL.so /usr/local/lib64/
# Alternatively, we could edit /usr/local/lib/cmake/Qt5Gui/Qt5GuiConfigExtras.cmake
# - _qt5gui_find_extra_libs(OPENGL "/usr/local/lib64/libGL.so" "" "")
# + _qt5gui_find_extra_libs(OPENGL "/usr/lib64/libGL.so" "" "")
