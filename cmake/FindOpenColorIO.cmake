#
# Copyright 2019 Pixar
# Modifications: Copyright (C) 2021 Olive Team
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

find_path(OCIO_BASE_DIR
        include/OpenColorIO/OpenColorABI.h
    HINTS
        "${OCIO_LOCATION}"
        "$ENV{OCIO_LOCATION}"
    DOC
        "OCIO root folder"
)

find_path(OCIO_INCLUDE_DIR
        OpenColorIO/OpenColorABI.h
    HINTS
        "${OCIO_LOCATION}"
        "$ENV{OCIO_LOCATION}"
        "${OCIO_BASE_DIR}"
    PATH_SUFFIXES
        include/
    DOC
        "OCIO headers path"
)

list(APPEND OCIO_INCLUDE_DIRS ${OCIO_INCLUDE_DIR})

find_library(OCIO_LIBRARY
    NAMES
        # libOpenColorIO.so.2.x (Linux)
        libOpenColorIO.so.2.1
        libOpenColorIO.so.2.0

        # libOpenColorIO.2.x.dylib (macOS)
        OpenColorIO.2.1
        OpenColorIO.2.0

        # OpenColorIO_2_x.lib (Windows)
        OpenColorIO_2_1
        OpenColorIO_2_0
    HINTS
        "${OCIO_LOCATION}"
        "$ENV{OCIO_LOCATION}"
        "${OCIO_BASE_DIR}"
    PATH_SUFFIXES
        lib/
    DOC
        "OCIO library path"
)

list(APPEND OCIO_LIBRARIES ${OCIO_LIBRARY})

if(OCIO_INCLUDE_DIRS AND EXISTS "${OCIO_INCLUDE_DIR}/OpenColorIO/OpenColorABI.h")
    file(STRINGS ${OCIO_INCLUDE_DIR}/OpenColorIO/OpenColorABI.h
        fullVersion
        REGEX
        "#define OCIO_VERSION .*$")
    string(REGEX MATCH "[0-9]+.[0-9]+.[0-9]+" OCIO_VERSION ${fullVersion})
endif()

# handle the QUIETLY and REQUIRED arguments and set OpenColorIO_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(OpenColorIO
    REQUIRED_VARS
        OCIO_LIBRARIES
        OCIO_INCLUDE_DIRS
    VERSION_VAR
        OCIO_VERSION
)
