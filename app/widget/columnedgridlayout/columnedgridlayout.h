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

#ifndef COLUMNEDGRIDLAYOUT_H
#define COLUMNEDGRIDLAYOUT_H

#include <QGridLayout>

#include "common/define.h"

namespace olive {

/**
 * @brief The ColumnedGridLayout class
 *
 * A simple derivative of QGridLayout that provides a automatic row/column layout based on a specified maximum
 * column count.
 */
class ColumnedGridLayout : public QGridLayout
{
  Q_OBJECT
public:
  ColumnedGridLayout(QWidget* parent = nullptr,
                     int maximum_columns = 0);

  void Add(QWidget* widget);
  int MaximumColumns() const;
  void SetMaximumColumns(int maximum_columns);

private:
  int maximum_columns_;
};

}

#endif // COLUMNEDGRIDLAYOUT_H
