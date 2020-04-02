#include "colorbutton.h"

#include "dialog/color/colordialog.h"

ColorButton::ColorButton(QWidget *parent) :
  QPushButton(parent)
{
  color_ = Color(1.0f, 1.0f, 1.0f);

  connect(this, &ColorButton::clicked, this, &ColorButton::ShowColorDialog);

  UpdateColor();
}

void ColorButton::ShowColorDialog()
{
  ColorDialog cd(nullptr, color_, this);

  if (cd.exec() == QDialog::Accepted) {
    color_ = cd.GetSelectedColor();
    UpdateColor();
  }
}

void ColorButton::UpdateColor()
{
  setStyleSheet(QStringLiteral("ColorButton {background: %1;}").arg(color_.toQColor().name()));
}
