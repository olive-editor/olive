#
# Copyright 2016 Pixar
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

if(UNIX)
    find_path(OIIO_BASE_DIR
            include/OpenImageIO/oiioversion.h
        HINTS
            "${OIIO_LOCATION}"
            "$ENV{OIIO_LOCATION}"
            "/opt/oiio"
    )
    find_path(OIIO_LIBRARY_DIR
            libOpenImageIO.so
        HINTS
            "${OIIO_LOCATION}"
            "$ENV{OIIO_LOCATION}"
            "${OIIO_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "OpenImageIO library path"
    )
elseif(WIN32)
    find_path(OIIO_BASE_DIR
            include/OpenImageIO/oiioversion.h
        HINTS
            "${OIIO_LOCATION}"
            "$ENV{OIIO_LOCATION}"
    )
    find_path(OIIO_LIBRARY_DIR
            OpenImageIO.lib
        HINTS
            "${OIIO_LOCATION}"
            "$ENV{OIIO_LOCATION}"
            "${OIIO_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "OpenImageIO library path"
    )
endif()

find_path(OIIO_INCLUDE_DIR
        OpenImageIO/oiioversion.h
    HINTS
        "${OIIO_LOCATION}"
        "$ENV{OIIO_LOCATION}"
        "${OIIO_BASE_DIR}"
    PATH_SUFFIXES
        include/
    DOC
        "OpenImageIO headers path"
)

list(APPEND OIIO_INCLUDE_DIRS ${OIIO_INCLUDE_DIR})

foreach(OIIO_LIB
    OpenImageIO
    OpenImageIO_Util
    )

    find_library(OIIO_${OIIO_LIB}_LIBRARY
            ${OIIO_LIB}
        HINTS
            "${OIIO_LOCATION}"
            "$ENV{OIIO_LOCATION}"
            "${OIIO_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "OIIO's ${OIIO_LIB} library path"
    )

    if(OIIO_${OIIO_LIB}_LIBRARY)
        list(APPEND OIIO_LIBRARIES ${OIIO_${OIIO_LIB}_LIBRARY})
    endif()
endforeach(OIIO_LIB)

foreach(OIIO_BIN
        iconvert
        idiff
        igrep
        iinfo
        iv
        maketx
        oiiotool)

    find_program(OIIO_${OIIO_BIN}_BINARY
            ${OIIO_BIN}
        HINTS
            "${OIIO_LOCATION}"
            "$ENV{OIIO_LOCATION}"
            "${OIIO_BASE_DIR}"
        PATH_SUFFIXES
            bin/
        DOC
            "OIIO's ${OIIO_BIN} binary"
    )
    if(OIIO_${OIIO_BIN}_BINARY)
        list(APPEND OIIO_BINARIES ${OIIO_${OIIO_BIN}_BINARY})
    endif()
endforeach(OIIO_BIN)

if(OIIO_INCLUDE_DIRS AND EXISTS "${OIIO_INCLUDE_DIR}/OpenImageIO/oiioversion.h")
    file(STRINGS ${OIIO_INCLUDE_DIR}/OpenImageIO/oiioversion.h
        MAJOR
        REGEX
        "#define OIIO_VERSION_MAJOR.*$")
    file(STRINGS ${OIIO_INCLUDE_DIR}/OpenImageIO/oiioversion.h
        MINOR
        REGEX
        "#define OIIO_VERSION_MINOR.*$")
    file(STRINGS ${OIIO_INCLUDE_DIR}/OpenImageIO/oiioversion.h
        PATCH
        REGEX
        "#define OIIO_VERSION_PATCH.*$")
    string(REGEX MATCHALL "[0-9]+" MAJOR ${MAJOR})
    string(REGEX MATCHALL "[0-9]+" MINOR ${MINOR})
    string(REGEX MATCHALL "[0-9]+" PATCH ${PATCH})
    set(OIIO_VERSION "${MAJOR}.${MINOR}.${PATCH}")
endif()

# handle the QUIETLY and REQUIRED arguments and set OIIO_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(OpenImageIO
    REQUIRED_VARS
        OIIO_LIBRARIES
        OIIO_INCLUDE_DIRS
    VERSION_VAR
        OIIO_VERSION
)
