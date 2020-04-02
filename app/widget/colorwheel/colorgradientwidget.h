#ifndef COLORGRADIENTGLWIDGET_H
#define COLORGRADIENTGLWIDGET_H

#include "colorswatchwidget.h"
#include "render/color.h"

class ColorGradientWidget : public ColorSwatchWidget
{
  Q_OBJECT
public:
  ColorGradientWidget(Qt::Orientation orientation, QWidget* parent = nullptr);

protected:
  virtual Color GetColorFromScreenPos(const QPoint& p) const override;

  virtual void paintEvent(QPaintEvent* e) override;

  virtual void SelectedColorChangedEvent(const Color& c, bool external) override;

private:
  static Color LerpColor(const Color& a, const Color& b, int i, int max);

  QPixmap cached_gradient_;

  Qt::Orientation orientation_;

  Color start_;

  Color end_;

  float val_;

};

#endif // COLORGRADIENTGLWIDGET_H
