# Olive - Non-Linear Video Editor
# Copyright (C) 2019 Olive Team
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

if(UNIX)
    find_path(OTIO_BASE_DIR
            include/opentimelineio/timeline.h
        HINTS
            "${OTIO_LOCATION}"
            "$ENV{OTIO_LOCATION}"
            "/opt/otio"
    )
    find_path(OTIO_LIBRARY_DIR
            libOpenTimelineIO.so
        HINTS
            "${OTIO_LOCATION}"
            "$ENV{OTIO_LOCATION}"
            "${OTIO_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "OpenTimelineIO library path"
    )
elseif(WIN32)
    find_path(OTIO_BASE_DIR
            include/opentimelineio/timeline.h
        HINTS
            "${OTIO_LOCATION}"
            "$ENV{OTIO_LOCATION}"
    )
    find_path(OTIO_LIBRARY_DIR
            OpenTimelineIO.lib
        HINTS
            "${OTIO_LOCATION}"
            "$ENV{OTIO_LOCATION}"
            "${OTIO_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "OpenTimelineIO library path"
    )
endif()

find_path(OTIO_INCLUDE_DIR
        OpenTimelineIO/timeline.h
    HINTS
        "${OTIO_LOCATION}"
        "$ENV{OTIO_LOCATION}"
        "${OTIO_BASE_DIR}"
    PATH_SUFFIXES
        include/
    DOC
        "OpenTimelineIO headers path"
)

list(APPEND OTIO_INCLUDE_DIRS ${OTIO_INCLUDE_DIR})

find_path(OTIO_DEPS_INCLUDE_DIR
        any/any.hpp
    HINTS
        "${OTIO_LOCATION}"
        "$ENV{OTIO_LOCATION}"
        "${OTIO_BASE_DIR}"
    PATH_SUFFIXES
        include/opentimelineio/deps/
    DOC
        "OpenTimelineIO headers path"
)

list(APPEND OTIO_INCLUDE_DIRS ${OTIO_DEPS_INCLUDE_DIR})

find_path(OT_INCLUDE_DIR
        OpenTime/rationalTime.h
    HINTS
        "${OTIO_LOCATION}"
        "$ENV{OTIO_LOCATION}"
        "${OTIO_BASE_DIR}"
    PATH_SUFFIXES
        include/
    DOC
        "OpenTime headers path"
)

list(APPEND OTIO_INCLUDE_DIRS ${OT_INCLUDE_DIR})

find_library(OTIO_LIBRARY
        OpenTimelineIO
    HINTS
        "${OTIO_LOCATION}"
        "$ENV{OTIO_LOCATION}"
        "${OTIO_BASE_DIR}"
    PATH_SUFFIXES
        lib/
    DOC
        "OTIO's ${OTIO_LIB} library path"
)

list(APPEND OTIO_LIBRARIES ${OTIO_LIBRARY})

find_library(OT_LIBRARY
        OpenTime
    HINTS
        "${OTIO_LOCATION}"
        "$ENV{OTIO_LOCATION}"
        "${OTIO_BASE_DIR}"
    PATH_SUFFIXES
        lib/
    DOC
        "OpenTime's ${OTIO_LIB} library path"
)

list(APPEND OTIO_LIBRARIES ${OT_LIBRARY})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(OpenTimelineIO
    REQUIRED_VARS
        OTIO_LIBRARIES
        OTIO_INCLUDE_DIRS
        OTIO_DEPS_INCLUDE_DIR
)
