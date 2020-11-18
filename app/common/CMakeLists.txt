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

set(OLIVE_SOURCES
  ${OLIVE_SOURCES}
  common/bezier.cpp
  common/bezier.h
  common/cancelableobject.h
  common/channellayout.h
  common/clamp.h
  common/commandlineparser.cpp
  common/commandlineparser.h
  common/crashpadinterface.cpp
  common/crashpadinterface.h
  common/crashpadutils.h
  common/debug.cpp
  common/debug.h
  common/define.h
  common/ffmpegutils.cpp
  common/ffmpegutils.h
  common/filefunctions.cpp
  common/filefunctions.h
  common/flipmodifiers.cpp
  common/flipmodifiers.h
  common/functiontimer.h
  common/lerp.h
  common/memorypool.cpp
  common/memorypool.h
  common/ocioutils.cpp
  common/ocioutils.h
  common/oiioutils.cpp
  common/oiioutils.h
  common/qtutils.cpp
  common/qtutils.h
  common/range.h
  common/ratiodialog.cpp
  common/ratiodialog.h
  common/rational.h
  common/rational.cpp
  common/threadsafemap.h
  common/threadedobject.cpp
  common/threadedobject.h
  common/timecodefunctions.cpp
  common/timecodefunctions.h
  common/timerange.cpp
  common/timerange.h
  common/tohex.h
  common/xmlutils.cpp
  common/xmlutils.h
  PARENT_SCOPE
)
