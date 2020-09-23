# Copyright (C) 2020 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

# Build image (default):
#  docker build -t olivevideoeditor/ci-package-ffmpeg:4.2.4 -f ci-ffmpeg/Dockerfile .

ARG OLIVE_ORG=olivevideoeditor
ARG CI_COMMON_VERSION=2
ARG FFMPEG_VERSION=4.2.4
ARG NASM_VERSION=2.15.03
ARG YASM_VERSION=1.3.0
ARG LAME_VERSION=3.100
ARG OPUS_VERSION=1.3.1
# TODO: Make version of x264, x265 and libvpx selectable?
ARG NUM_JOBS=2

FROM ${OLIVE_ORG}/ci-common:${CI_COMMON_VERSION} as ci-ffmpeg

ARG OLIVE_ORG
ARG CI_COMMON_VERSION
ARG FFMPEG_VERSION
ARG NASM_VERSION
ARG YASM_VERSION
ARG LAME_VERSION
ARG OPUS_VERSION
# TODO: Make version of x264, x265 and libvpx selectable?
ARG NUM_JOBS=2

LABEL maintainer="olivevideoeditor@gmail.com"

LABEL org.opencontainers.image.name="$OLIVE_ORG/ci-ffmpeg"
LABEL org.opencontainers.image.description="CentOS FFmpeg Build Image"
LABEL org.opencontainers.image.url="http://olivevideoeditor.org"
LABEL org.opencontainers.image.source="https://github.com/olive-editor/olive"
LABEL org.opencontainers.image.vendor="Olive Team"
LABEL org.opencontainers.image.version="1.0"

COPY scripts/build_ffmpeg.sh \
     /tmp/

ENV OLIVE_ORG=${OLIVE_ORG} \
    CI_COMMON_VERSION=${CI_COMMON_VERSION} \
    FFMPEG_VERSION=${FFMPEG_VERSION} \
    NASM_VERSION=${NASM_VERSION} \
    YASM_VERSION=${YASM_VERSION} \
    LAME_VERSION=${LAME_VERSION} \
    OPUS_VERSION=${OPUS_VERSION} \
    NUM_JOBS=${NUM_JOBS} \
    OLIVE_INSTALL_PREFIX=/usr/local

RUN /tmp/before_build.sh && \
    /tmp/build_ffmpeg.sh && \
    /tmp/copy_new_files.sh

FROM scratch as ci-package-ffmpeg

COPY --from=ci-ffmpeg /package/. /
