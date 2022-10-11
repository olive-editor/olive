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
  widget/timelinewidget/undo/timelineundocommon.h
  widget/timelinewidget/undo/timelineundogeneral.cpp
  widget/timelinewidget/undo/timelineundogeneral.h
  widget/timelinewidget/undo/timelineundopointer.cpp
  widget/timelinewidget/undo/timelineundopointer.h
  widget/timelinewidget/undo/timelineundoripple.cpp
  widget/timelinewidget/undo/timelineundoripple.h
  widget/timelinewidget/undo/timelineundosplit.cpp
  widget/timelinewidget/undo/timelineundosplit.h
  widget/timelinewidget/undo/timelineundotrack.cpp
  widget/timelinewidget/undo/timelineundotrack.h
  widget/timelinewidget/undo/timelineundoworkarea.cpp
  widget/timelinewidget/undo/timelineundoworkarea.h
  PARENT_SCOPE
)
