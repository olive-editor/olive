# Copyright (C) 2020 Olive Team
# Copyright (c) Contributors to the aswf-docker Project. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-or-later

# Build image (default):
#  docker build -t olivevideoeditor/ci-common:2 -f ci-common/Dockerfile .

ARG OLIVE_ORG=olivevideoeditor
ARG ASWF_PKG_ORG=aswftesting
ARG CI_COMMON_VERSION=2
ARG DTS_VERSION=9
ARG CMAKE_VERSION=3.19.3

FROM ${ASWF_PKG_ORG}/ci-package-clang:${CI_COMMON_VERSION} as ci-package-clang
FROM ${ASWF_PKG_ORG}/ci-package-ninja:${CI_COMMON_VERSION} as ci-package-ninja

FROM centos:7 as ci-common

ARG OLIVE_ORG
ARG CI_COMMON_VERSION
ARG DTS_VERSION
ARG CMAKE_VERSION

LABEL maintainer="olivevideoeditor@gmail.com"
LABEL org.opencontainers.image.name="$OLIVE_ORG/ci-common"
LABEL org.opencontainers.image.description="CentOS CI Shared Image"
LABEL org.opencontainers.image.url="http://olivevideoeditor.org"
LABEL org.opencontainers.image.source="https://github.com/olive-editor/olive"
LABEL org.opencontainers.image.vendor="Olive Team"
LABEL org.opencontainers.image.version="1.0"
LABEL org.opencontainers.image.licenses="GPL-3.0-or-later"

USER root

COPY scripts/common/install_yumpackages.sh \
     scripts/common/before_build.sh \
     scripts/common/copy_new_files.sh \
     /tmp/

ENV DTS_VERSION=${DTS_VERSION}
RUN /tmp/install_yumpackages.sh

RUN mkdir /opt/olive
WORKDIR /opt/olive

ENV OLIVE_ORG=${OLIVE_ORG} \
    CI_COMMON_VERSION=${CI_COMMON_VERSION} \
    CMAKE_VERSION=${CMAKE_VERSION} \
    LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib64:/opt/rh/httpd24/root/usr/lib64:/opt/rh/devtoolset-${DTS_VERSION}/root/usr/lib64:/opt/rh/devtoolset-${DTS_VERSION}/root/usr/lib:${LD_LIBRARY_PATH} \
    PATH=/opt/rh/rh-git218/root/usr/bin:/usr/local/bin:/opt/rh/devtoolset-${DTS_VERSION}/root/usr/bin:/opt/app-root/src/bin:/opt/rh/devtoolset-${DTS_VERSION}/root/usr/bin/:/usr/local/sbin:/usr/sbin:/usr/bin:/sbin:/bin

#COPY scripts/common/install_sonar.sh \
#     scripts/common/install_ccache.sh \
#     /tmp/

COPY --from=ci-package-clang /. /usr/local/
COPY --from=ci-package-ninja /. /usr/local/

#COPY scripts/common/setup_aswfuser.sh /tmp
#RUN /tmp/setup_aswfuser.sh

COPY scripts/base/install_cmake.sh \
     /tmp/

RUN export DOWNLOADS_DIR=/tmp/downloads && \
    mkdir /tmp/downloads && \
#   source /tmp/versions_base.sh && \
    /tmp/install_cmake.sh && \
#   /tmp/patchup.sh && \
    rm -rf /tmp/downloads
# TODO: Remove scripts from /tmp ?
