#ifndef COLORWHEELWIDGET_H
#define COLORWHEELWIDGET_H

#include <QOpenGLWidget>

#include "colorswatchwidget.h"
#include "render/backend/opengl/openglshader.h"
#include "render/color.h"

class ColorWheelWidget : public ColorSwatchWidget
{
  Q_OBJECT
public:
  ColorWheelWidget(QWidget* parent = nullptr);

signals:
  void DiameterChanged(int radius);

protected:
  virtual Color GetColorFromScreenPos(const QPoint& p) const override;

  virtual void resizeEvent(QResizeEvent* e) override;

  virtual void paintEvent(QPaintEvent* e) override;

  virtual void SelectedColorChangedEvent(const Color& c) override;

private:
  int GetDiameter() const;

  qreal GetRadius() const;

  struct Triangle {
    qreal opposite;
    qreal adjacent;
    qreal hypotenuse;
  };

  Triangle GetTriangleFromCoords(const QPoint &center, const QPoint& p) const;
  Triangle GetTriangleFromCoords(const QPoint &center, qreal y, qreal x) const;

  Color GetColorFromTriangle(const Triangle& tri) const;
  QPoint GetCoordsFromColor(const Color& c) const;

  QPixmap cached_wheel_;

  float val_;

  bool force_redraw_;

};

#endif // COLORWHEELWIDGET_H
