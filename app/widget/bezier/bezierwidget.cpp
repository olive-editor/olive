/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "bezierwidget.h"

#include <QGridLayout>
#include <QGroupBox>

namespace olive {

BezierWidget::BezierWidget(QWidget *parent) :
  QWidget{parent}
{
  QGridLayout *layout = new QGridLayout(this);

  int row = 0;

  layout->addWidget(new QLabel(tr("Center:")), row, 0);

  x_slider_ = new FloatSlider();
  connect(x_slider_, &FloatSlider::ValueChanged, this, &BezierWidget::ValueChanged);
  layout->addWidget(x_slider_, row, 1);

  y_slider_ = new FloatSlider();
  connect(y_slider_, &FloatSlider::ValueChanged, this, &BezierWidget::ValueChanged);
  layout->addWidget(y_slider_, row, 2);

  row++;

  QGroupBox *bezier_group = new QGroupBox(tr("Bezier"));
  layout->addWidget(bezier_group, row, 0, 1, 3);

  QGridLayout *bezier_layout = new QGridLayout(bezier_group);

  row = 0;

  bezier_layout->addWidget(new QLabel(tr("In:")), row, 0);

  cp1_x_slider_ = new FloatSlider();
  connect(cp1_x_slider_, &FloatSlider::ValueChanged, this, &BezierWidget::ValueChanged);
  bezier_layout->addWidget(cp1_x_slider_, row, 1);

  cp1_y_slider_ = new FloatSlider();
  connect(cp1_y_slider_, &FloatSlider::ValueChanged, this, &BezierWidget::ValueChanged);
  bezier_layout->addWidget(cp1_y_slider_, row, 2);

  row++;

  bezier_layout->addWidget(new QLabel(tr("Out:")), row, 0);

  cp2_x_slider_ = new FloatSlider();
  connect(cp2_x_slider_, &FloatSlider::ValueChanged, this, &BezierWidget::ValueChanged);
  bezier_layout->addWidget(cp2_x_slider_, row, 1);

  cp2_y_slider_ = new FloatSlider();
  connect(cp2_y_slider_, &FloatSlider::ValueChanged, this, &BezierWidget::ValueChanged);
  bezier_layout->addWidget(cp2_y_slider_, row, 2);
}

Bezier BezierWidget::GetValue() const
{
  Bezier b;

  b.set_x(x_slider_->GetValue());
  b.set_y(y_slider_->GetValue());
  b.set_cp1_x(cp1_x_slider_->GetValue());
  b.set_cp1_y(cp1_y_slider_->GetValue());
  b.set_cp2_x(cp2_x_slider_->GetValue());
  b.set_cp2_y(cp2_y_slider_->GetValue());

  return b;
}

void BezierWidget::SetValue(const Bezier &b)
{
  x_slider_->SetValue(b.x());
  y_slider_->SetValue(b.y());
  cp1_x_slider_->SetValue(b.cp1_x());
  cp1_y_slider_->SetValue(b.cp1_y());
  cp2_x_slider_->SetValue(b.cp2_x());
  cp2_y_slider_->SetValue(b.cp2_y());
}

}
