# Copyright (C) 2020 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

# Build image (default):
#  docker build -t olivevideoeditor/ci-olive:2021.2 -f ci-olive/Dockerfile .

ARG OLIVE_ORG=olivevideoeditor
ARG ASWF_PKG_ORG=aswftesting
ARG CI_COMMON_VERSION=2
ARG VFXPLATFORM_VERSION=2021
ARG FFMPEG_VERSION=4.2.4

FROM ${ASWF_PKG_ORG}/ci-package-qt:${VFXPLATFORM_VERSION} as ci-package-qt
FROM ${ASWF_PKG_ORG}/ci-package-python:${VFXPLATFORM_VERSION} as ci-package-python
FROM ${ASWF_PKG_ORG}/ci-package-boost:${VFXPLATFORM_VERSION} as ci-package-boost
FROM ${ASWF_PKG_ORG}/ci-package-openexr:${VFXPLATFORM_VERSION} as ci-package-openexr
FROM ${ASWF_PKG_ORG}/ci-package-oiio:${VFXPLATFORM_VERSION} as ci-package-oiio
FROM ${ASWF_PKG_ORG}/ci-package-ocio:${VFXPLATFORM_VERSION} as ci-package-ocio
FROM ${OLIVE_ORG}/ci-package-ffmpeg:${FFMPEG_VERSION} as ci-package-ffmpeg
FROM ${OLIVE_ORG}/ci-package-crashpad:latest as ci-package-crashpad

FROM ${OLIVE_ORG}/ci-common:${CI_COMMON_VERSION} as ci-olive

ARG OLIVE_ORG
ARG VFXPLATFORM_VERSION
ARG PYTHON_VERSION=3.7

LABEL maintainer="olivevideoeditor@gmail.com"
LABEL com.vfxplatform.version=$VFXPLATFORM_VERSION
LABEL org.opencontainers.image.name="olivevideoeditor/ci-olive"
LABEL org.opencontainers.image.description="CentOS CI Olive Build Image"
LABEL org.opencontainers.image.url="http://olivevideoeditor.org"
LABEL org.opencontainers.image.source="https://github.com/olive-editor/olive"
LABEL org.opencontainers.image.vendor="Olive Team"
LABEL org.opencontainers.image.version="1.0"

ENV PYTHONPATH=/usr/local/lib/python${PYTHON_VERSION}/site-packages:${PYTHONPATH} \
    CRASHPAD_LOCATION=/usr/local/crashpad \
    VFXPLATFORM_VERSION=${VFXPLATFORM_VERSION}

COPY --from=ci-package-qt /. /usr/local/
COPY --from=ci-package-python /. /usr/local/
COPY --from=ci-package-boost /. /usr/local/
COPY --from=ci-package-openexr /. /usr/local/
COPY --from=ci-package-oiio /. /usr/local/
COPY --from=ci-package-ocio /. /usr/local/
COPY --from=ci-package-ffmpeg /. /usr/local/
COPY --from=ci-package-crashpad /. /usr/local/

RUN curl --location "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" \
      -o "/usr/local/linuxdeployqt-x86_64.AppImage" && \
    chmod a+x "/usr/local/linuxdeployqt-x86_64.AppImage"
