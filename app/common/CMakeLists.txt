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

set(OLIVE_SOURCES
  ${OLIVE_SOURCES}
  common/channellayout.h
  common/clamp.h
  common/debug.h
  common/debug.cpp
  common/filefunctions.h
  common/filefunctions.cpp
  common/lerp.h
  common/range.h
  common/rational.h
  common/rational.cpp
  common/qobjectlistcast.h
  common/qtversionabstraction.h
  common/qtversionabstraction.cpp
  common/threadedobject.h
  common/threadedobject.cpp
  common/timecodefunctions.h
  common/timecodefunctions.cpp
  PARENT_SCOPE
)
