#ifndef COLORWHEELWIDGET_H
#define COLORWHEELWIDGET_H

#include <QOpenGLWidget>

#include "colorswatchwidget.h"
#include "render/backend/opengl/openglshader.h"
#include "render/color.h"

class ColorWheelGLWidget : public ColorSwatchWidget
{
  Q_OBJECT
public:
  ColorWheelGLWidget(QWidget* parent = nullptr);

  void SetHSVValue(float val);

signals:
  void DiameterChanged(int radius);

protected:
  virtual Color GetColorFromCursorPos(const QPoint& p) const override;

  virtual bool PointIsValid(const QPoint& p) const override;

  virtual OpenGLShader* CreateShader() const override;

  virtual void SetShaderUniformValues(OpenGLShader* shader) const override;

  virtual void resizeEvent(QResizeEvent* e) override;

private:
  int GetDiameter() const;

  qreal GetRadius() const;

  float hsv_value_;

};

/*class ColorWheelWidget : public QWidget
{
public:
  ColorWheelWidget(QWidget* parent = nullptr);

protected:
  virtual void paintEvent(QPaintEvent* e) override;

};*/

#endif // COLORWHEELWIDGET_H
