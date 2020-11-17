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

#include "columnedgridlayout.h"

namespace olive {

ColumnedGridLayout::ColumnedGridLayout(QWidget* parent,
                                       int maximum_columns) :
  QGridLayout (parent),
  maximum_columns_(maximum_columns)
{
}

void ColumnedGridLayout::Add(QWidget *widget)
{
  if (maximum_columns_ > 0) {

    int row = count() / maximum_columns_;
    int column = count() % maximum_columns_;

    addWidget(widget, row, column);

  } else {

    addWidget(widget);

  }
}

int ColumnedGridLayout::MaximumColumns() const
{
  return maximum_columns_;
}

void ColumnedGridLayout::SetMaximumColumns(int maximum_columns)
{
  maximum_columns_ = maximum_columns;
}

}
