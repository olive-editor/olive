# Copyright (C) 2020 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

# Build image (default):
#  docker build -t olivevideoeditor/ci-package-ocio:2021-2.0 -f ci-ocio/Dockerfile .

ARG OLIVE_ORG=olivevideoeditor
ARG ASWF_PKG_ORG=aswftesting
ARG CI_COMMON_VERSION=2
ARG VFXPLATFORM_VERSION=2021
# Latest configs ~3 GB because of ACES 1.0.x.
# Upstream only copies nuke-default, therefore we also use the older configs.
ARG OCIO_CONFIGS_VERSION=1.0_r2

#FROM ${ASWF_PKG_ORG}/ci-ocio:${VFXPLATFORM_VERSION} as ci-ocio
FROM ${OLIVE_ORG}/ci-common:${CI_COMMON_VERSION} as ci-ocio

ARG OLIVE_ORG
ARG CI_COMMON_VERSION
ARG VFXPLATFORM_VERSION
ARG OCIO_CONFIGS_VERSION

LABEL maintainer="olivevideoeditor@gmail.com"
LABEL com.vfxplatform.version=$VFXPLATFORM_VERSION
LABEL org.opencontainers.image.name="$OLIVE_ORG/ci-otio"
LABEL org.opencontainers.image.description="CentOS OpenColorIO Build Image"
LABEL org.opencontainers.image.url="http://olivevideoeditor.org"
LABEL org.opencontainers.image.source="https://github.com/olive-editor/olive"
LABEL org.opencontainers.image.vendor="Olive Team"
LABEL org.opencontainers.image.version="1.0"
LABEL org.opencontainers.image.licenses="GPL-3.0-or-later"

ENV OLIVE_ORG=${OLIVE_ORG} \
    CI_COMMON_VERSION=${CI_COMMON_VERSION} \
    VFXPLATFORM_VERSION=${VFXPLATFORM_VERSION} \
    OCIO_CONFIGS_VERSION=${OCIO_CONFIGS_VERSION} \
    OLIVE_INSTALL_PREFIX=/usr/local

COPY scripts/build_ocio.sh \
#    scripts/common/before_build.sh \
#    scripts/common/copy_new_files.sh \
     /tmp/

RUN /tmp/before_build.sh && \
    /tmp/build_ocio.sh && \
    /tmp/copy_new_files.sh

FROM scratch as ci-package-ocio

COPY --from=ci-ocio /package/. /
