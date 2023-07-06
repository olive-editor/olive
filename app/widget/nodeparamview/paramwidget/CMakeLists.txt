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
  widget/nodeparamview/paramwidget/abstractparamwidget.cpp
  widget/nodeparamview/paramwidget/abstractparamwidget.h
  widget/nodeparamview/paramwidget/arrayparamwidget.cpp
  widget/nodeparamview/paramwidget/arrayparamwidget.h
  widget/nodeparamview/paramwidget/bezierparamwidget.cpp
  widget/nodeparamview/paramwidget/bezierparamwidget.h
  widget/nodeparamview/paramwidget/boolparamwidget.cpp
  widget/nodeparamview/paramwidget/boolparamwidget.h
  widget/nodeparamview/paramwidget/colorparamwidget.cpp
  widget/nodeparamview/paramwidget/colorparamwidget.h
  widget/nodeparamview/paramwidget/comboparamwidget.cpp
  widget/nodeparamview/paramwidget/comboparamwidget.h
  widget/nodeparamview/paramwidget/fileparamwidget.cpp
  widget/nodeparamview/paramwidget/fileparamwidget.h
  widget/nodeparamview/paramwidget/floatsliderparamwidget.cpp
  widget/nodeparamview/paramwidget/floatsliderparamwidget.h
  widget/nodeparamview/paramwidget/fontparamwidget.cpp
  widget/nodeparamview/paramwidget/fontparamwidget.h
  widget/nodeparamview/paramwidget/integersliderparamwidget.cpp
  widget/nodeparamview/paramwidget/integersliderparamwidget.h
  widget/nodeparamview/paramwidget/numericsliderparamwidget.cpp
  widget/nodeparamview/paramwidget/numericsliderparamwidget.h
  widget/nodeparamview/paramwidget/rationalsliderparamwidget.cpp
  widget/nodeparamview/paramwidget/rationalsliderparamwidget.h
  widget/nodeparamview/paramwidget/textparamwidget.cpp
  widget/nodeparamview/paramwidget/textparamwidget.h
  PARENT_SCOPE
)
