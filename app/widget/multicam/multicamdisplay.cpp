/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "multicamdisplay.h"

namespace olive {

#define super ViewerDisplayWidget

MulticamDisplay::MulticamDisplay(QWidget *parent) :
  super(parent),
  node_(nullptr)
{
}

void MulticamDisplay::OnPaint()
{
  super::OnPaint();

  if (node_) {
    QPainter p(paint_device());

    p.setPen(QPen(Qt::yellow, fontMetrics().height()/4));
    p.setBrush(Qt::NoBrush);

    int rows, cols;
    node_->GetRowsAndColumns(&rows, &cols);

    int multi = std::max(rows, cols);
    int cell_width = width() / multi;
    int cell_height = height() / multi;

    int col, row;
    node_->IndexToRowCols(node_->GetCurrentSource(), rows, cols, &row, &col);

    QRect r(cell_width * col, cell_height * row, cell_width, cell_height);
    p.drawRect(GenerateWorldTransform().mapRect(r));
  }
}

void MulticamDisplay::SetMulticamNode(MultiCamNode *n)
{
  node_ = n;
}

}
