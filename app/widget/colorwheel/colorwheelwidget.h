#ifndef COLORWHEELWIDGET_H
#define COLORWHEELWIDGET_H

#include <QWidget>

class ColorWheelWidget : public QWidget
{
public:
  ColorWheelWidget(QWidget* parent = nullptr);

protected:
  virtual void paintEvent(QPaintEvent* event) override;

};

#endif // COLORWHEELWIDGET_H
