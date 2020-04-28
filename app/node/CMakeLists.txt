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

add_subdirectory(audio)
add_subdirectory(block)
add_subdirectory(generator)
add_subdirectory(input)
add_subdirectory(math)
add_subdirectory(output)

set(OLIVE_SOURCES
  ${OLIVE_SOURCES}
  node/dependency.h
  node/dependency.cpp
  node/edge.h
  node/edge.cpp
  node/external.h
  node/external.cpp
  node/factory.h
  node/factory.cpp
  node/graph.h
  node/graph.cpp
  node/input.h
  node/input.cpp
  node/inputarray.h
  node/inputarray.cpp
  node/keyframe.h
  node/keyframe.cpp
  node/metareader.h
  node/metareader.cpp
  node/node.h
  node/node.cpp
  node/output.h
  node/output.cpp
  node/param.h
  node/param.cpp
  node/traverser.h
  node/traverser.cpp
  node/value.h
  node/value.cpp
  PARENT_SCOPE
)
