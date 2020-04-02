#ifndef COLORGRADIENTGLWIDGET_H
#define COLORGRADIENTGLWIDGET_H

#include "colorswatchwidget.h"
#include "render/color.h"

class ColorGradientGLWidget : public ColorSwatchWidget
{
  Q_OBJECT
public:
  ColorGradientGLWidget(Qt::Orientation orientation, QWidget* parent = nullptr);

  void SetColors(const Color& a, const Color& b);

protected:
  virtual Color GetColorFromCursorPos(const QPoint& p) const override;

  virtual bool PointIsValid(const QPoint& p) const override;

  virtual OpenGLShader* CreateShader() const override;

  virtual void SetShaderUniformValues(OpenGLShader* shader) const override;

private:
  Qt::Orientation orientation_;

  Color a_;

  Color b_;

};

#endif // COLORGRADIENTGLWIDGET_H
