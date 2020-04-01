#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <QPushButton>

class ColorButton : public QPushButton
{
  Q_OBJECT
public:
  ColorButton(QWidget* parent = nullptr);

signals:
  void ColorChanged();

private slots:
  void ShowColorDialog();

};

#endif // COLORBUTTON_H
