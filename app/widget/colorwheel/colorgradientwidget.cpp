#include "colorgradientwidget.h"

#include "common/lerp.h"
#include "node/node.h"

ColorGradientGLWidget::ColorGradientGLWidget(Qt::Orientation orientation, QWidget *parent) :
  ColorSwatchWidget(parent),
  orientation_(orientation)
{
}

void ColorGradientGLWidget::SetColors(const Color &a, const Color &b)
{
  a_ = a;
  b_ = b;

  update();
}

Color ColorGradientGLWidget::GetColorFromCursorPos(const QPoint &p) const
{
  float t;

  if (orientation_ == Qt::Horizontal) {
    t = static_cast<float>(p.x()) / static_cast<float>(width());
  } else {
    t = static_cast<float>(p.y()) / static_cast<float>(height());
  }

  return Color(lerp(a_.red(), b_.red(), t),
               lerp(a_.green(), b_.green(), t),
               lerp(a_.blue(), b_.blue(), t));
}

bool ColorGradientGLWidget::PointIsValid(const QPoint &) const
{
  return true;
}

OpenGLShader *ColorGradientGLWidget::CreateShader() const
{
  OpenGLShader* s = new OpenGLShader();

  s->create();
  s->addShaderFromSourceCode(QOpenGLShader::Vertex, OpenGLShader::CodeDefaultVertex());
  s->addShaderFromSourceCode(QOpenGLShader::Fragment, Node::ReadFileAsString(QStringLiteral(":/shaders/colorgradient.frag")));
  s->link();

  return s;
}

void ColorGradientGLWidget::SetShaderUniformValues(OpenGLShader *shader) const
{
  shader->setUniformValue("orientation", orientation_);
  shader->setUniformValue("color_a", a_.red(), a_.green(), a_.blue());
  shader->setUniformValue("color_b", b_.red(), b_.green(), b_.blue());
}
