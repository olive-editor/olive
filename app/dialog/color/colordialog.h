#ifndef COLORDIALOG_H
#define COLORDIALOG_H

#include <QDialog>

#include "colorvalueswidget.h"
#include "render/color.h"
#include "render/colormanager.h"
#include "widget/colorwheel/colorwheelwidget.h"
#include "widget/colorwheel/colorgradientwidget.h"

class ColorDialog : public QDialog
{
  Q_OBJECT
public:
  ColorDialog(ColorManager* color_manager, Color start = Color(1.0f, 1.0f, 1.0f), QWidget* parent = nullptr);

private:
  ColorManager* color_manager_;

  ColorWheelGLWidget* color_wheel_;

  ColorGradientGLWidget* hsv_value_gradient_;

private slots:
  void UpdateValueGradient(const Color& c);

  void UpdateWheelValue(const Color& c);

  void UpdateValueGradientSize(int diameter);

};

#endif // COLORDIALOG_H
