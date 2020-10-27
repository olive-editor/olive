# Copyright (C) 2020 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

# Build image (default):
#  docker build -t olivevideoeditor/ci-package-otio -f ci-otio/Dockerfile .

ARG OLIVE_ORG=olivevideoeditor
ARG ASWF_PKG_ORG=aswftesting
ARG CI_COMMON_VERSION=2
ARG VFXPLATFORM_VERSION=2021

FROM ${ASWF_PKG_ORG}/ci-package-python:${VFXPLATFORM_VERSION} as ci-package-python
FROM ${ASWF_PKG_ORG}/ci-package-qt:${VFXPLATFORM_VERSION} as ci-package-qt
FROM ${ASWF_PKG_ORG}/ci-package-pyside:${VFXPLATFORM_VERSION} as ci-package-pyside

FROM ${OLIVE_ORG}/ci-common:${CI_COMMON_VERSION} as ci-otio

ARG OLIVE_ORG
ARG VFXPLATFORM_VERSION
ARG PYTHON_VERSION=3.7

LABEL maintainer="olivevideoeditor@gmail.com"
LABEL com.vfxplatform.version=$VFXPLATFORM_VERSION
LABEL org.opencontainers.image.name="$OLIVE_ORG/ci-otio"
LABEL org.opencontainers.image.description="CentOS OpenTimelineIO Build Image"
LABEL org.opencontainers.image.url="http://olivevideoeditor.org"
LABEL org.opencontainers.image.source="https://github.com/olive-editor/olive"
LABEL org.opencontainers.image.vendor="Olive Team"
LABEL org.opencontainers.image.version="1.0"
LABEL org.opencontainers.image.licenses="GPL-3.0-or-later"

ENV OLIVE_ORG=${OLIVE_ORG} \
    CI_COMMON_VERSION=${CI_COMMON_VERSION} \
    VFXPLATFORM_VERSION=${VFXPLATFORM_VERSION} \
    PYTHONPATH=/usr/local/lib/python${PYTHON_VERSION}/site-packages:/usr/local/lib/python \
    OLIVE_INSTALL_PREFIX=/usr/local

COPY scripts/base/install_cmake.sh \
     scripts/build_otio.sh \
     /tmp/

COPY --from=ci-package-python /. /usr/local/
COPY --from=ci-package-qt /. /usr/local/
COPY --from=ci-package-pyside /. /usr/local/

RUN export DOWNLOADS_DIR=/tmp/downloads && \
    mkdir /tmp/downloads && \
    /tmp/install_cmake.sh && \
    pip install flake8 tox urllib3 && \
    rm -rf /tmp/downloads

RUN /tmp/before_build.sh && \
    /tmp/build_otio.sh && \
    /tmp/copy_new_files.sh

FROM scratch as ci-package-otio

COPY --from=ci-otio /package/. /
