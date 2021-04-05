# Copyright (C) 2020 Olive Team
# SPDX-License-Identifier: GPL-3.0-or-later

# Build image (default):
#  docker build -t olivevideoeditor/ci-package-crashpad:centos8 -f ci-crashpad/Dockerfile --build-arg CI_COMMON_VERSION=2-centos8 .

ARG OLIVE_ORG=olivevideoeditor
ARG CI_COMMON_VERSION=2
ARG ASWF_PKG_ORG=aswftesting
ARG VFXPLATFORM_VERSION=2019

# Crashpad does not appear to support Python 3, try ASWF Python 2.7
FROM ${ASWF_PKG_ORG}/ci-package-python:${VFXPLATFORM_VERSION} as ci-package-python

FROM ${OLIVE_ORG}/ci-common:${CI_COMMON_VERSION} as ci-crashpad

ARG OLIVE_ORG
ARG CI_COMMON_VERSION
ARG PYTHON_VERSION=2.7

LABEL maintainer="olivevideoeditor@gmail.com"
LABEL org.opencontainers.image.name="$OLIVE_ORG/ci-crashpad"
LABEL org.opencontainers.image.description="CentOS Crashpad Build Image"
LABEL org.opencontainers.image.url="http://olivevideoeditor.org"
LABEL org.opencontainers.image.source="https://github.com/olive-editor/olive"
LABEL org.opencontainers.image.vendor="Olive Team"
LABEL org.opencontainers.image.version="1.0"
LABEL org.opencontainers.image.licenses="GPL-3.0-or-later"

COPY scripts/build_crashpad.sh \
     /tmp/

COPY --from=ci-package-python /. /usr/local/

# Python hashlib module requires libssl
RUN dnf install -y openssl openssl-devel openssl-libs
# hash functions still not available in virtuelenv...

ENV OLIVE_ORG=${OLIVE_ORG} \
    CI_COMMON_VERSION=${CI_COMMON_VERSION} \
    OLIVE_INSTALL_PREFIX=/usr/local \
    LD_LIBRARY_PATH=/usr/lib64:/usr/lib:/usr/lib64/python2.7/lib-dynload/:${LD_LIBRARY_PATH}
# Unsetting PYTHONPATH does not seem to help with
#   ValueError: unsupported hash type md5
#   ERROR:root:code for hash sha1 was not found.
# https://stackoverflow.com/questions/13239578/virtualenv-system-site-packages-not-using-system-site-packages
#   PYTHONPATH=/usr/local/lib/python${PYTHON_VERSION}/site-packages:/usr/local/lib64/python${PYTHON_VERSION}/site-packages:/usr/local/lib/python:${PYTHONPATH} \

#RUN dnf install -y python2 && \
#    cd /usr/lib && \
#    ln -s python2 python && \
#    ln -s pip2 pip

#ENV PYTHONPATH=/usr/lib/python2.7/site-packages:/usr/lib/python

RUN /tmp/before_build.sh && \
    /tmp/build_crashpad.sh && \
    /tmp/copy_new_files.sh

FROM scratch as ci-package-crashpad

COPY --from=ci-crashpad /package /
