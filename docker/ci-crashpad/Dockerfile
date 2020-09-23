# Copyright (C) 2020 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

# Build image (default):
#  docker build -t olivevideoeditor/ci-package-crashpad -f ci-crashpad/Dockerfile .

ARG OLIVE_ORG=olivevideoeditor
ARG CI_COMMON_VERSION=2

FROM ${OLIVE_ORG}/ci-common:${CI_COMMON_VERSION} as ci-crashpad

ARG OLIVE_ORG
ARG CI_COMMON_VERSION

LABEL maintainer="olivevideoeditor@gmail.com"

LABEL org.opencontainers.image.name="$OLIVE_ORG/ci-crashpad"
LABEL org.opencontainers.image.description="CentOS Crashpad Build Image"
LABEL org.opencontainers.image.url="http://olivevideoeditor.org"
LABEL org.opencontainers.image.source="https://github.com/olive-editor/olive"
LABEL org.opencontainers.image.vendor="Olive Team"
LABEL org.opencontainers.image.version="1.0"

COPY scripts/build_crashpad.sh \
     /tmp/

ENV OLIVE_ORG=${OLIVE_ORG} \
    CI_COMMON_VERSION=${CI_COMMON_VERSION} \
    OLIVE_INSTALL_PREFIX=/usr/local

RUN /tmp/before_build.sh && \
    /tmp/build_crashpad.sh && \
    /tmp/copy_new_files.sh

FROM scratch as ci-package-crashpad

COPY --from=ci-crashpad /package /
