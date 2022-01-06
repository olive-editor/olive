# Olive - Non-Linear Video Editor
# Copyright (C) 2021 Olive Team
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
  widget/nodeview/nodeview.cpp
  widget/nodeview/nodeview.h
  widget/nodeview/nodeviewcommon.h
  widget/nodeview/nodeviewcontext.cpp
  widget/nodeview/nodeviewcontext.h
  widget/nodeview/nodeviewedge.cpp
  widget/nodeview/nodeviewedge.h
  widget/nodeview/nodeviewitem.cpp
  widget/nodeview/nodeviewitem.h
  widget/nodeview/nodeviewitemconnector.cpp
  widget/nodeview/nodeviewitemconnector.h
  widget/nodeview/nodeviewminimap.cpp
  widget/nodeview/nodeviewminimap.h
  widget/nodeview/nodeviewscene.cpp
  widget/nodeview/nodeviewscene.h
  widget/nodeview/nodeviewtoolbar.cpp
  widget/nodeview/nodeviewtoolbar.h
  widget/nodeview/nodeviewundo.cpp
  widget/nodeview/nodeviewundo.h
  widget/nodeview/nodewidget.cpp
  widget/nodeview/nodewidget.h
  PARENT_SCOPE
)
