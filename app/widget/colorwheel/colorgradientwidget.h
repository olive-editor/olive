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

#ifndef COLORGRADIENTGLWIDGET_H
#define COLORGRADIENTGLWIDGET_H

#include "colorswatchwidget.h"
#include "render/color.h"

namespace olive {

class ColorGradientWidget : public ColorSwatchWidget
{
  Q_OBJECT
public:
  ColorGradientWidget(Qt::Orientation orientation, QWidget* parent = nullptr);

protected:
  virtual Color GetColorFromScreenPos(const QPoint& p) const override;

  virtual void paintEvent(QPaintEvent* e) override;

  virtual void SelectedColorChangedEvent(const Color& c, bool external) override;

private:
  static Color LerpColor(const Color& a, const Color& b, int i, int max);

  QPixmap cached_gradient_;

  Qt::Orientation orientation_;

  Color start_;

  Color end_;

  double val_;

};

}

#endif // COLORGRADIENTGLWIDGET_H
