#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <QPushButton>

#include "render/color.h"

class ColorButton : public QPushButton
{
  Q_OBJECT
public:
  ColorButton(QWidget* parent = nullptr);

signals:
  void ColorChanged();

private slots:
  void ShowColorDialog();

private:
  Color color_;

};

#endif // COLORBUTTON_H
