#include "colorbutton.h"

#include "dialog/color/colordialog.h"

ColorButton::ColorButton(QWidget *parent) :
  QPushButton(parent)
{
  connect(this, &ColorButton::clicked, this, &ColorButton::ShowColorDialog);
}

void ColorButton::ShowColorDialog()
{
  ColorDialog cd(nullptr);
  cd.exec();
}
