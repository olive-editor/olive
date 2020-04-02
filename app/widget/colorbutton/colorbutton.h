#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <QPushButton>

#include "render/color.h"
#include "render/colormanager.h"

class ColorButton : public QPushButton
{
  Q_OBJECT
public:
  ColorButton(ColorManager* color_manager, QWidget* parent = nullptr);

signals:
  void ColorChanged();

private slots:
  void ShowColorDialog();

private:
  void UpdateColor();

  ColorManager* color_manager_;

  Color color_;

};

#endif // COLORBUTTON_H
