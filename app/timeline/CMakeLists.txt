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

set(OLIVE_SOURCES
  ${OLIVE_SOURCES}
  timeline/timelinecommon.h
  timeline/timelinecoordinate.h
  timeline/timelinecoordinate.cpp
  timeline/timelinemarker.h
  timeline/timelinemarker.cpp
  timeline/timelineundocommon.h
  timeline/timelineundogeneral.cpp
  timeline/timelineundogeneral.h
  timeline/timelineundopointer.cpp
  timeline/timelineundopointer.h
  timeline/timelineundoripple.cpp
  timeline/timelineundoripple.h
  timeline/timelineundosplit.cpp
  timeline/timelineundosplit.h
  timeline/timelineundotrack.cpp
  timeline/timelineundotrack.h
  timeline/timelineworkarea.h
  timeline/timelineworkarea.cpp
  PARENT_SCOPE
)
