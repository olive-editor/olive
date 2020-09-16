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

include(FindPackageHandleStandardArgs)

# Try to find include files
find_path(CRASHPAD_CLIENT_INCLUDE_DIR
        client/crashpad_client.h
    HINTS
        "${CRASHPAD_LOCATION}"
        "$ENV{CRASHPAD_LOCATION}"
        "${CRASHPAD_BASE_DIR}"
)
list(APPEND CRASHPAD_INCLUDE_DIRS ${CRASHPAD_CLIENT_INCLUDE_DIR})

find_path(CRASHPAD_BASE_INCLUDE_DIR
        base/files/file_path.h
    HINTS
        "${CRASHPAD_LOCATION}"
        "$ENV{CRASHPAD_LOCATION}"
        "${CRASHPAD_BASE_DIR}"
    PATH_SUFFIXES
        "third_party/mini_chromium/mini_chromium"
)
list(APPEND CRASHPAD_INCLUDE_DIRS ${CRASHPAD_BASE_INCLUDE_DIR})

# Try to find build files
if (WIN32)
  find_path(CRASHPAD_LIBRARY_DIRS
          obj/client/client.lib
      HINTS
          "${CRASHPAD_LOCATION}"
          "$ENV{CRASHPAD_LOCATION}"
          "${CRASHPAD_BASE_DIR}"
      PATH_SUFFIXES
          "out/Default"
  )
elseif(UNIX)
  # Assuming macOS works this way, don't actually know
  find_path(CRASHPAD_LIBRARY_DIRS
          obj/client/libclient.a
      HINTS
          "${CRASHPAD_LOCATION}"
          "$ENV{CRASHPAD_LOCATION}"
          "${CRASHPAD_BASE_DIR}"
      PATH_SUFFIXES
          "out/Default"
  )
endif()

# Find the libraries we need
set (_crashpad_components
  client/client
  util/util
  third_party/mini_chromium/mini_chromium/base/base)
foreach (COMPONENT ${_crashpad_components})
  string(REGEX MATCH "^(.*[\\\/])" SUBDIR ${COMPONENT})
  string(REGEX MATCH "([^\/]+$)" SHORT_COMPONENT ${COMPONENT})
  string(TOUPPER ${SHORT_COMPONENT} UPPERCOMPONENT)

  find_library(CRASHPAD_${UPPERCOMPONENT}_LIB
                ${SHORT_COMPONENT}
                HINTS "${CRASHPAD_LIBRARY_DIRS}/obj/${SUBDIR}"
  )

  list(APPEND CRASHPAD_LIBRARIES ${CRASHPAD_${UPPERCOMPONENT}_LIB})
endforeach()

find_package_handle_standard_args(GoogleCrashpad
    REQUIRED_VARS
        CRASHPAD_LIBRARIES
        CRASHPAD_INCLUDE_DIRS
)
