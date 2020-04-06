/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef COLORVALUESWIDGET_H
#define COLORVALUESWIDGET_H

#include <QWidget>

#include "colorpreviewbox.h"
#include "render/color.h"
#include "widget/slider/floatslider.h"

OLIVE_NAMESPACE_ENTER

class ColorValuesWidget : public QWidget
{
  Q_OBJECT
public:
  ColorValuesWidget(QWidget* parent = nullptr);

  Color GetColor() const;

  ColorPreviewBox* preview_box() const;

public slots:
  void SetColor(const Color& c);

signals:
  void ColorChanged(const Color& c);

private:
  FloatSlider* CreateColorSlider();

  ColorPreviewBox* preview_;

  FloatSlider* red_slider_;
  FloatSlider* green_slider_;
  FloatSlider* blue_slider_;

private slots:
  void SliderChanged();

};

OLIVE_NAMESPACE_EXIT

#endif // COLORVALUESWIDGET_H
