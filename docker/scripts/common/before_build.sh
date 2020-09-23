#!/usr/bin/env bash
# Copyright (c) Contributors to the aswf-docker Project. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

set -ex

rm -rf /package

cd "${OLIVE_INSTALL_PREFIX}"
find . -type f -o -type l | cut -c3- > /tmp/previous-prefix-files.txt
