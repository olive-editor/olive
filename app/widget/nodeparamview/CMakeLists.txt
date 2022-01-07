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
  widget/nodeparamview/nodeparamview.cpp
  widget/nodeparamview/nodeparamview.h
  widget/nodeparamview/nodeparamviewarraywidget.cpp
  widget/nodeparamview/nodeparamviewarraywidget.h
  widget/nodeparamview/nodeparamviewconnectedlabel.cpp
  widget/nodeparamview/nodeparamviewconnectedlabel.h
  widget/nodeparamview/nodeparamviewcontext.cpp
  widget/nodeparamview/nodeparamviewcontext.h
  widget/nodeparamview/nodeparamviewdockarea.cpp
  widget/nodeparamview/nodeparamviewdockarea.h
  widget/nodeparamview/nodeparamviewitem.cpp
  widget/nodeparamview/nodeparamviewitem.h
  widget/nodeparamview/nodeparamviewitembase.cpp
  widget/nodeparamview/nodeparamviewitembase.h
  widget/nodeparamview/nodeparamviewitemtitlebar.cpp
  widget/nodeparamview/nodeparamviewitemtitlebar.h
  widget/nodeparamview/nodeparamviewkeyframecontrol.cpp
  widget/nodeparamview/nodeparamviewkeyframecontrol.h
  widget/nodeparamview/nodeparamviewtextedit.cpp
  widget/nodeparamview/nodeparamviewtextedit.h
  widget/nodeparamview/nodeparamviewundo.cpp
  widget/nodeparamview/nodeparamviewundo.h
  widget/nodeparamview/nodeparamviewwidgetbridge.cpp
  widget/nodeparamview/nodeparamviewwidgetbridge.h
  PARENT_SCOPE
)
