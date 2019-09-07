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

add_subdirectory(blend)
add_subdirectory(block)
add_subdirectory(color)
add_subdirectory(distort)
add_subdirectory(generator)
add_subdirectory(input)
add_subdirectory(output)
add_subdirectory(processor)

set(OLIVE_SOURCES
  ${OLIVE_SOURCES}
  node/dependency.h
  node/dependency.cpp
  node/edge.h
  node/edge.cpp
  node/graph.h
  node/graph.cpp
  node/input.h
  node/input.cpp
  node/keyframe.h
  node/keyframe.cpp
  node/menu.h
  node/menu.cpp
  node/node.h
  node/node.cpp
  node/output.h
  node/output.cpp
  node/param.h
  node/param.cpp
  PARENT_SCOPE
)
