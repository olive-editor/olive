# Copyright (C) 2020 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

# Build image (default):
#  docker build -t olivevideoeditor/ci-package-ffmpeg:4.2.4 -f ci-ffmpeg/Dockerfile .

ARG OLIVE_ORG=olivevideoeditor
ARG CI_COMMON_VERSION=2
ARG FFMPEG_VERSION=4.2.4
ARG NASM_VERSION=2.15.03
ARG YASM_VERSION=1.3.0
ARG AOM_VERSION=v2.0.1
ARG X265_VERSION=3.4
ARG OGG_VERSION=1.3.4
ARG OPUS_VERSION=1.3.1
ARG VORBIS_VERSION=1.3.7
ARG THEORA_VERSION=1.1.1
ARG VPX_VERSION=1.9.0
ARG WEBP_VERSION=1.1.0
ARG LAME_VERSION=3.100
ARG XVID_VERSION=1.3.5
ARG OPENJPEG_VERSION=2.3.1
ARG LIBPNG_VERSION=1.6.31
# TODO: Make version of x264 selectable?
ARG NUM_JOBS=2

FROM ${OLIVE_ORG}/ci-common:${CI_COMMON_VERSION} as ci-ffmpeg

ARG OLIVE_ORG
ARG CI_COMMON_VERSION
ARG FFMPEG_VERSION
ARG NASM_VERSION
ARG YASM_VERSION
ARG AOM_VERSION
ARG X265_VERSION
ARG OGG_VERSION
ARG OPUS_VERSION
ARG VORBIS_VERSION
ARG THEORA_VERSION
ARG VPX_VERSION
ARG WEBP_VERSION
ARG LAME_VERSION
ARG XVID_VERSION
ARG OPENJPEG_VERSION
ARG LIBPNG_VERSION
ARG NUM_JOBS

LABEL maintainer="olivevideoeditor@gmail.com"
LABEL org.opencontainers.image.name="$OLIVE_ORG/ci-ffmpeg"
LABEL org.opencontainers.image.description="CentOS FFmpeg Build Image"
LABEL org.opencontainers.image.url="http://olivevideoeditor.org"
LABEL org.opencontainers.image.source="https://github.com/olive-editor/olive"
LABEL org.opencontainers.image.vendor="Olive Team"
LABEL org.opencontainers.image.version="1.0"
LABEL org.opencontainers.image.licenses="GPL-3.0-or-later"

COPY scripts/build_ffmpeg.sh \
     /tmp/

ENV OLIVE_ORG=${OLIVE_ORG} \
    CI_COMMON_VERSION=${CI_COMMON_VERSION} \
    FFMPEG_VERSION=${FFMPEG_VERSION} \
    NASM_VERSION=${NASM_VERSION} \
    YASM_VERSION=${YASM_VERSION} \
    AOM_VERSION=${AOM_VERSION} \
    X265_VERSION=${X265_VERSION} \
    OGG_VERSION=${OGG_VERSION} \
    OPUS_VERSION=${OPUS_VERSION} \
    VORBIS_VERSION=${VORBIS_VERSION} \
    THEORA_VERSION=${THEORA_VERSION} \
    VPX_VERSION=${VPX_VERSION} \
    WEBP_VERSION=${WEBP_VERSION} \
    LAME_VERSION=${LAME_VERSION} \
    XVID_VERSION=${XVID_VERSION} \
    OPENJPEG_VERSION=${OPENJPEG_VERSION} \
    LIBPNG_VERSION=${LIBPNG_VERSION} \
    NUM_JOBS=${NUM_JOBS} \
    OLIVE_INSTALL_PREFIX=/usr/local

RUN /tmp/before_build.sh && \
    /tmp/build_ffmpeg.sh && \
    /tmp/copy_new_files.sh

FROM scratch as ci-package-ffmpeg

COPY --from=ci-ffmpeg /package/. /
