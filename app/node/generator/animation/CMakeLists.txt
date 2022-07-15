# Olive - Non-Linear Video Editor
# Copyright (C) 2022 Olive Team
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

set(OLIVE_SOURCES
  ${OLIVE_SOURCES}
  node/generator/animation/textanimation.h
  node/generator/animation/textanimationengine.cpp
  node/generator/animation/textanimationengine.h
  node/generator/animation/textanimationxmlformatter.cpp
  node/generator/animation/textanimationxmlformatter.h
  node/generator/animation/textanimationxmlparser.cpp
  node/generator/animation/textanimationxmlparser.h
  node/generator/animation/textanimationnode.cpp
  node/generator/animation/textanimationnode.h
  node/generator/animation/textanimationrendernode.cpp
  node/generator/animation/textanimationrendernode.h
  PARENT_SCOPE
)
