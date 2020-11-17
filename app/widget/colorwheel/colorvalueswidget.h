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

#ifndef COLORVALUESWIDGET_H
#define COLORVALUESWIDGET_H

#include <QWidget>

#include "colorpreviewbox.h"
#include "render/color.h"
#include "render/colormanager.h"
#include "widget/slider/floatslider.h"

OLIVE_NAMESPACE_ENTER

class ColorValuesTab : public QWidget
{
  Q_OBJECT
public:
  ColorValuesTab(QWidget* parent = nullptr);

  Color GetColor() const;

  void SetColor(const Color& c);

signals:
  void ColorChanged(const Color& c);

private:
  FloatSlider* CreateColorSlider();

  FloatSlider* red_slider_;
  FloatSlider* green_slider_;
  FloatSlider* blue_slider_;

private slots:
  void SliderChanged();

};

class ColorValuesWidget : public QWidget
{
  Q_OBJECT
public:
  ColorValuesWidget(ColorManager* manager, QWidget* parent = nullptr);

  Color GetColor() const;

  void SetColorProcessor(ColorProcessorPtr input_to_ref,
                         ColorProcessorPtr ref_to_display,
                         ColorProcessorPtr display_to_ref,
                         ColorProcessorPtr ref_to_input);

public slots:
  void SetColor(const Color& c);

signals:
  void ColorChanged(const Color& c);

private:
  void UpdateInputFromRef();

  void UpdateDisplayFromRef();

  void UpdateRefFromInput();

  void UpdateRefFromDisplay();

  ColorManager* manager_;

  ColorPreviewBox* preview_;

  ColorValuesTab* input_tab_;

  ColorValuesTab* reference_tab_;

  ColorValuesTab* display_tab_;

  ColorProcessorPtr input_to_ref_;

  ColorProcessorPtr ref_to_display_;

  ColorProcessorPtr display_to_ref_;

  ColorProcessorPtr ref_to_input_;

private slots:
  void UpdateValuesFromInput();

  void UpdateValuesFromRef();

  void UpdateValuesFromDisplay();

};

OLIVE_NAMESPACE_EXIT

#endif // COLORVALUESWIDGET_H
