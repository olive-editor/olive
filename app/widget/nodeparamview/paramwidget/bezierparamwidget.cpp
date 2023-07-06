/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "bezierparamwidget.h"

#include "widget/bezier/bezierwidget.h"

namespace olive {

BezierParamWidget::BezierParamWidget(QObject *parent)
  : AbstractParamWidget{parent}
{

}

void BezierParamWidget::Initialize(QWidget *parent, size_t channels)
{
  Q_ASSERT(channels == 6);

  BezierWidget *bezier = new BezierWidget(parent);
  AddWidget(bezier);

  connect(bezier->x_slider(), &FloatSlider::ValueChanged, this, &BezierParamWidget::Arbitrate);
  connect(bezier->y_slider(), &FloatSlider::ValueChanged, this, &BezierParamWidget::Arbitrate);
  connect(bezier->cp1_x_slider(), &FloatSlider::ValueChanged, this, &BezierParamWidget::Arbitrate);
  connect(bezier->cp1_y_slider(), &FloatSlider::ValueChanged, this, &BezierParamWidget::Arbitrate);
  connect(bezier->cp2_x_slider(), &FloatSlider::ValueChanged, this, &BezierParamWidget::Arbitrate);
  connect(bezier->cp2_y_slider(), &FloatSlider::ValueChanged, this, &BezierParamWidget::Arbitrate);
}

void BezierParamWidget::SetValue(const value_t &val)
{
  Bezier b(val.value<double>(0), val.value<double>(1), val.value<double>(2), val.value<double>(3), val.value<double>(4), val.value<double>(5));
  BezierWidget* bw = static_cast<BezierWidget*>(GetWidgets().at(0));
  bw->SetValue(b);
}

void BezierParamWidget::Arbitrate()
{
  // Widget is a FloatSlider (child of BezierWidget)
  BezierWidget *bw = static_cast<BezierWidget*>(GetWidgets().at(0));
  FloatSlider *fs = static_cast<FloatSlider*>(sender());

  int index = -1;
  if (fs == bw->x_slider()) {
    index = 0;
  } else if (fs == bw->y_slider()) {
    index = 1;
  } else if (fs == bw->cp1_x_slider()) {
    index = 2;
  } else if (fs == bw->cp1_y_slider()) {
    index = 3;
  } else if (fs == bw->cp2_x_slider()) {
    index = 4;
  } else if (fs == bw->cp2_y_slider()) {
    index = 5;
  }

  if (index != -1) {
    emit SliderDragged(fs, index);
  }
}

}
