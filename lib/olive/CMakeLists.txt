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

add_subdirectory(src)

add_library(olivecore
  ${OLIVECORE_SOURCES}
)

target_include_directories(olivecore PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/app"
)
target_link_libraries(olivecore PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets)

if (OpenTimelineIO_FOUND)
  target_compile_definitions(olivecore PRIVATE USE_OTIO)
  target_include_directories(olivecore PRIVATE ${OTIO_INCLUDE_DIRS})
  target_link_libraries(olivecore PRIVATE ${OTIO_LIBRARIES})
endif()
