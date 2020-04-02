#ifndef COLORVALUESWIDGET_H
#define COLORVALUESWIDGET_H

#include <QWidget>

#include "colorpreviewbox.h"
#include "render/color.h"
#include "widget/slider/floatslider.h"

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

#endif // COLORVALUESWIDGET_H
