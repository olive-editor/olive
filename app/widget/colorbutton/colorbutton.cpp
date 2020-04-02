#include "colorbutton.h"

#include "dialog/color/colordialog.h"

ColorButton::ColorButton(QWidget *parent) :
  QPushButton(parent)
{
  color_ = Color(1.0f, 1.0f, 1.0f);

  connect(this, &ColorButton::clicked, this, &ColorButton::ShowColorDialog);
}

void ColorButton::ShowColorDialog()
{
  ColorDialog cd(nullptr, color_, this);
  cd.exec();
}
