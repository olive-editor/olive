#ifndef COLORSWATCHWIDGET_H
#define COLORSWATCHWIDGET_H

#include <QOpenGLWidget>

#include "render/backend/opengl/openglshader.h"
#include "render/color.h"

class ColorSwatchWidget : public QOpenGLWidget
{
  Q_OBJECT
public:
  ColorSwatchWidget(QWidget* parent = nullptr);

  virtual ~ColorSwatchWidget() override;

signals:
  void ColorChanged(const Color& c);

protected:
  virtual void initializeGL() override;

  virtual void paintGL() override;

  virtual void mousePressEvent(QMouseEvent* e) override;

  virtual void mouseMoveEvent(QMouseEvent* e) override;

  virtual void SetShaderUniformValues(OpenGLShader*) const {}

  virtual Color GetColorFromCursorPos(const QPoint& p) const = 0;

  virtual bool PointIsValid(const QPoint& p) const = 0;

  virtual OpenGLShader* CreateShader() const = 0;

private slots:
  void CleanUp();

private:
  OpenGLShader* shader_;

};

#endif // COLORSWATCHWIDGET_H
