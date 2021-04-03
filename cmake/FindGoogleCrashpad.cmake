# Olive - Non-Linear Video Editor
# Copyright (C) 2020 Olive Team
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

find_path(CRASHPAD_BUILD_INCLUDE_DIR
    build/chromeos_buildflags.h
  HINTS
    "${CRASHPAD_LOCATION}"
    "$ENV{CRASHPAD_LOCATION}"
    "${CRASHPAD_BASE_DIR}"
  PATH_SUFFIXES
    "out/Default/gen"
)
list(APPEND CRASHPAD_INCLUDE_DIRS ${CRASHPAD_BUILD_INCLUDE_DIR})

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
  client
  util
  third_party/mini_chromium/mini_chromium/base
)

set (_crashpad_required
  CRASHPAD_CLIENT_LIB
  CRASHPAD_UTIL_LIB
  CRASHPAD_BASE_LIB
  BREAKPAD_BIN_DIR
  CRASHPAD_BUILD_INCLUDE_DIR
  CRASHPAD_CLIENT_INCLUDE_DIR
  CRASHPAD_BASE_INCLUDE_DIR
)

if (WIN32 OR (UNIX AND NOT APPLE))
  set (_crashpad_components
    ${_crashpad_components}
    compat
  )

  set (_crashpad_required
    ${_crashpad_required}
    CRASHPAD_COMPAT_LIB
  )
endif()

foreach (COMPONENT ${_crashpad_components})
  get_filename_component(SHORT_COMPONENT ${COMPONENT} NAME)
  string(TOUPPER ${SHORT_COMPONENT} UPPER_COMPONENT)

  find_library(CRASHPAD_${UPPER_COMPONENT}_LIB
      ${SHORT_COMPONENT}
    HINTS
      "${CRASHPAD_LIBRARY_DIRS}/obj/${COMPONENT}"
  )

  list(APPEND CRASHPAD_LIBRARIES ${CRASHPAD_${UPPER_COMPONENT}_LIB})
endforeach()

if (APPLE)
  # macOS requires a bunch of extra loose object files that aren't made into static libraries
  # See: https://groups.google.com/a/chromium.org/g/crashpad-dev/c/XVggc7kvlNs/m/msMjHS4KAQAJ
  set (_crashpad_mach_loose_objects
    child_portServer
    #mach_excServer
    child_portUser
    #mach_excUser
    excServer
    notifyServer
    excUser
    notifyUser
  )

  foreach (COMPONENT ${_crashpad_mach_loose_objects})
    string(TOUPPER ${COMPONENT} UPPER_COMPONENT)
    set(LIB_NAME CRASHPAD_MACH_${UPPER_COMPONENT}_LIB)
    find_file(${LIB_NAME}
      NAMES
        mig_output.${COMPONENT}.o
      HINTS
        "${CRASHPAD_LIBRARY_DIRS}/obj/out/Default/gen/util/mach"
    )

    set (_crashpad_required
      ${_crashpad_required}
      ${LIB_NAME}
    )

    list(APPEND CRASHPAD_LIBRARIES ${${LIB_NAME}})
  endforeach()

  list(APPEND CRASHPAD_LIBRARIES
    bsm
    "-framework IOKit"
    "-framework Foundation"
    "-framework Security"
    "-framework CoreFoundation"
    "-framework ApplicationServices"
  )
endif()

# Find Breakpad's minidump_stackwalk
find_path(BREAKPAD_BIN_DIR
  "minidump_stackwalk${CMAKE_EXECUTABLE_SUFFIX}"
HINTS
  "${BREAKPAD_LOCATION}"
  "$ENV{BREAKPAD_LOCATION}"
  "${BREAKPAD_BASE_DIR}"
PATH_SUFFIXES
  bin
)

find_package_handle_standard_args(GoogleCrashpad
  REQUIRED_VARS
  ${_crashpad_required}
)

if (UNIX AND NOT APPLE)
  list(APPEND CRASHPAD_LIBRARIES
    ${CMAKE_DL_LIBS} # Crashpad compat lib needs libdl.so (-ldl)
    Threads::Threads # Link against libpthread.so (-lpthread)
  )
endif()
