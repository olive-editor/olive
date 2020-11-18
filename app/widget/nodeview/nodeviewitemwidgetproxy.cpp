/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "nodeviewitemwidgetproxy.h"

QColor NodeViewItemWidget::TitleBarColor()
{
  return title_bar_color_;
}

void NodeViewItemWidget::SetTitleBarColor(QColor color)
{
  title_bar_color_ = color;
}

QColor NodeViewItemWidget::BorderColor()
{
  return border_color_;
}

void NodeViewItemWidget::SetBorderColor(QColor color)
{
  border_color_ = color;
}
