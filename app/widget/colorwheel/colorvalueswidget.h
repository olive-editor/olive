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

#ifndef COLORVALUESWIDGET_H
#define COLORVALUESWIDGET_H

#include <QCheckBox>
#include <QPushButton>
#include <QWidget>

#include "colorpreviewbox.h"
#include "node/color/colormanager/colormanager.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/stringslider.h"

namespace olive {

class ColorValuesTab : public QWidget
{
  Q_OBJECT
public:
  ColorValuesTab(bool with_legacy_option = false, QWidget* parent = nullptr);

  Color GetColor() const;

  void SetColor(const Color& c);

  double GetRed() const;
  double GetGreen() const;
  double GetBlue() const;
  void SetRed(double r);
  void SetGreen(double g);
  void SetBlue(double b);

signals:
  void ColorChanged(const Color& c);

private:
  static const double kLegacyMultiplier;

  double GetValueInternal(FloatSlider *slider) const;
  void SetValueInternal(FloatSlider *slider, double v);

  bool AreSlidersLegacyValues() const;

  FloatSlider* CreateColorSlider();

  FloatSlider* red_slider_;
  FloatSlider* green_slider_;
  FloatSlider* blue_slider_;

  QLabel *hex_lbl_;
  StringSlider *hex_slider_;

  QVector<FloatSlider*> sliders_;

  QCheckBox *legacy_box_;

private slots:
  void SliderChanged();

  void LegacyChanged(bool e);

  void UpdateHex();

  void HexChanged(const QString &s);

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

  virtual bool eventFilter(QObject *watcher, QEvent *event) override;

  void IgnorePickFrom(QWidget *w)
  {
    ignore_pick_from_.append(w);
  }

public slots:
  void SetColor(const Color& c);

  void SetReferenceColor(const Color& c);

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

  QPushButton *color_picker_btn_;

  Color picker_end_color_;

  QVector<QWidget *> ignore_pick_from_;

private slots:
  void UpdateValuesFromInput();

  void UpdateValuesFromRef();

  void UpdateValuesFromDisplay();

  void ColorPickedBtnToggled(bool e);

};

}

#endif // COLORVALUESWIDGET_H
