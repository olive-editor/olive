# Olive - Non-Linear Video Editor
# Copyright (C) 2023 Olive Studios LLC
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

set(LIBOLIVE_COMPONENTS
  Core
  #Codec
)

foreach (COMPONENT ${LIBOLIVE_COMPONENTS})
  string(TOLOWER ${COMPONENT} LOWER_COMPONENT)
  string(TOUPPER ${COMPONENT} UPPER_COMPONENT)

  # Find include directory for this component
  find_path(LIBOLIVE_${UPPER_COMPONENT}_INCLUDEDIR
      olive/${LOWER_COMPONENT}/${LOWER_COMPONENT}.h
    HINTS
      "${LIBOLIVE_LOCATION}"
      "$ENV{LIBOLIVE_LOCATION}"
      "${LIBOLIVE_ROOT}"
      "$ENV{LIBOLIVE_ROOT}"
    PATH_SUFFIXES
      include/
  )

  find_library(LIBOLIVE_${UPPER_COMPONENT}_LIBRARY
      olive${LOWER_COMPONENT}
    HINTS
      "${LIBOLIVE_LOCATION}"
      "$ENV{LIBOLIVE_LOCATION}"
      "${LIBOLIVE_ROOT}"
      "$ENV{LIBOLIVE_ROOT}"
    PATH_SUFFIXES
      lib/
  )

  list(APPEND LIBOLIVE_LIBRARIES ${LIBOLIVE_${UPPER_COMPONENT}_LIBRARY})
  list(APPEND LIBOLIVE_INCLUDE_DIRS ${LIBOLIVE_${UPPER_COMPONENT}_INCLUDEDIR})
endforeach()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Olive
  REQUIRED_VARS
    LIBOLIVE_LIBRARIES
    LIBOLIVE_INCLUDE_DIRS
)
