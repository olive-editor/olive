# Copyright (C) 2022 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

# Build image (default):
#  docker build -t olivevideoeditor/ci-package-oiio:2.3.13.0 -f ci-oiio/Dockerfile .

ARG ASWF_PKG_ORG=aswftesting
ARG OLIVE_ORG=olivevideoeditor
ARG CI_COMMON_VERSION=2
ARG VFXPLATFORM_VERSION=2022
ARG OIIO_VERSION=v2.3.13.0
ARG YASM_VERSION=1.3.0
ARG JPEG_TURBO_VERSION=2.1.3

FROM ${ASWF_PKG_ORG}/ci-package-boost:${VFXPLATFORM_VERSION} as ci-package-boost
FROM ${ASWF_PKG_ORG}/ci-package-openexr:${VFXPLATFORM_VERSION} as ci-package-openexr
FROM ${ASWF_PKG_ORG}/ci-package-imath:${VFXPLATFORM_VERSION} as ci-package-imath

FROM ${OLIVE_ORG}/ci-common:${CI_COMMON_VERSION} as ci-oiio

ARG OLIVE_ORG
ARG VFXPLATFORM_VERSION
ARG OIIO_VERSION
ARG YASM_VERSION
ARG JPEG_TURBO_VERSION

LABEL maintainer="olivevideoeditor@gmail.com"
LABEL com.vfxplatform.version=$VFXPLATFORM_VERSION
LABEL org.opencontainers.image.name="$OLIVE_ORG/ci-oiio"
LABEL org.opencontainers.image.description="CentOS OpenImageIO Build Image"
LABEL org.opencontainers.image.url="http://olivevideoeditor.org"
LABEL org.opencontainers.image.source="https://github.com/olive-editor/olive"
LABEL org.opencontainers.image.vendor="Olive Team"
LABEL org.opencontainers.image.version="1.0"
LABEL org.opencontainers.image.licenses="GPL-3.0-or-later"

ENV OLIVE_ORG=${OLIVE_ORG} \
    VFXPLATFORM_VERSION=${VFXPLATFORM_VERSION} \
    OIIO_VERSION=${OIIO_VERSION} \
    YASM_VERSION=${YASM_VERSION} \
    JPEG_TURBO_VERSION=${JPEG_TURBO_VERSION} \
    OLIVE_INSTALL_PREFIX=/usr/local

COPY scripts/build_oiio.sh \
     /tmp/

COPY --from=ci-package-boost /. /usr/local/
COPY --from=ci-package-openexr /. /usr/local/
COPY --from=ci-package-imath /. /usr/local/

RUN curl -fLsS -o yasm.tar.gz "http://www.tortall.net/projects/yasm/releases/yasm-${YASM_VERSION}.tar.gz" && \
    tar xf yasm.tar.gz && \
    rm -f yasm.tar.gz && \
    cd yasm* && \
    ./configure --prefix="${OLIVE_INSTALL_PREFIX}" && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rf yasm*

RUN /tmp/before_build.sh && \
    /tmp/build_oiio.sh && \
    /tmp/copy_new_files.sh

FROM scratch as ci-package-oiio

COPY --from=ci-oiio /package/. /
