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

#ifndef BEZIERWIDGET_H
#define BEZIERWIDGET_H

#include <QCheckBox>
#include <QWidget>

#include "common/bezier.h"
#include "widget/slider/floatslider.h"

namespace olive {

class BezierWidget : public QWidget
{
  Q_OBJECT
public:
  explicit BezierWidget(QWidget *parent = nullptr);

  Bezier GetValue() const;

  void SetValue(const Bezier &b);

  FloatSlider *x_slider() const { return x_slider_; }

  FloatSlider *y_slider() const { return y_slider_; }

  FloatSlider *cp1_x_slider() const { return cp1_x_slider_; }

  FloatSlider *cp1_y_slider() const { return cp1_y_slider_; }

  FloatSlider *cp2_x_slider() const { return cp2_x_slider_; }

  FloatSlider *cp2_y_slider() const { return cp2_y_slider_; }

signals:
  void ValueChanged();

private:
  FloatSlider *x_slider_;

  FloatSlider *y_slider_;

  FloatSlider *cp1_x_slider_;

  FloatSlider *cp1_y_slider_;

  FloatSlider *cp2_x_slider_;

  FloatSlider *cp2_y_slider_;

};

}

#endif // BEZIERWIDGET_H
