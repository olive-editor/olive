# Copyright (C) 2022 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

# Build image (default):
#  docker build -t olivevideoeditor/ci-package-otio:0.15 -f ci-otio/Dockerfile .

ARG OLIVE_ORG=olivevideoeditor
ARG ASWF_PKG_ORG=aswftesting
ARG CI_COMMON_VERSION=2
ARG VFXPLATFORM_VERSION=2022
ARG OTIO_VERSION=v0.15
ARG CXX_STANDARD=17

# We are currently not interested in the Python implementation and bindings
#FROM ${ASWF_PKG_ORG}/ci-package-python:${VFXPLATFORM_VERSION} as ci-package-python
#FROM ${ASWF_PKG_ORG}/ci-package-qt:${VFXPLATFORM_VERSION} as ci-package-qt
#FROM ${ASWF_PKG_ORG}/ci-package-pyside:${VFXPLATFORM_VERSION} as ci-package-pyside

# Annoyingly, OTIO has Imath as a submodule and prefers it over external libs.
# This can clash with our Imath and OpenEXR versions, causing a linker error like:
#  framehashcache.cpp:(.text+0x206b): undefined reference to `Imf_3_1::Header::Header(int, int, float, Imath_3_2::Vec2<float> const&, float, Imf_3_1::LineOrder, Imf_3_1::Compression)'
# We need our Imath for building OTIO and make it prefer ours.
FROM ${ASWF_PKG_ORG}/ci-package-imath:${VFXPLATFORM_VERSION} as ci-package-imath

FROM ${OLIVE_ORG}/ci-common:${CI_COMMON_VERSION} as ci-otio

ARG OLIVE_ORG
ARG VFXPLATFORM_VERSION
ARG OTIO_VERSION
ARG CXX_STANDARD
ARG PYTHON_VERSION=3.9

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
    OTIO_VERSION=${OTIO_VERSION} \
    CXX_STANDARD=${CXX_STANDARD} \
    OLIVE_INSTALL_PREFIX=/usr/local
#   PYTHONPATH=/usr/local/lib/python${PYTHON_VERSION}/site-packages:/usr/local/lib/python \

COPY scripts/build_otio.sh \
     /tmp/

#COPY --from=ci-package-python /. /usr/local/
#COPY --from=ci-package-qt /. /usr/local/
#COPY --from=ci-package-pyside /. /usr/local/

COPY --from=ci-package-imath /. /usr/local/

# HACK: The ASWF Imath is compiled with Python support that we ignore to avoid an error:
#  set_property could not find TARGET Imath::PyImath_Python3_9.
COPY scripts/DisableImathPythonRelease.patch /tmp/
RUN patch -u /usr/local/lib64/cmake/Imath/ImathTargets-release.cmake /tmp/DisableImathPythonRelease.patch

RUN /tmp/before_build.sh && \
    /tmp/build_otio.sh && \
    /tmp/copy_new_files.sh

FROM scratch as ci-package-otio

COPY --from=ci-otio /package/. /
