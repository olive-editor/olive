#include "colorwheelwidget.h"

#include <QtMath>

#include "node/node.h"

#define M_180_OVER_PI 57.295791433133264917914229473464

ColorWheelGLWidget::ColorWheelGLWidget(QWidget *parent) :
  ColorSwatchWidget(parent),
  hsv_value_(1.0f)
{
}

void ColorWheelGLWidget::SetHSVValue(float val)
{
  hsv_value_ = val;
  update();
}

Color ColorWheelGLWidget::GetColorFromCursorPos(const QPoint &p) const
{
  QPoint center = rect().center();

  qreal opposite = p.y() - center.y();
  qreal adjacent = p.x() - center.x();
  qreal distance = qSqrt(qPow(adjacent, 2) + qPow(opposite, 2));

  qreal hue = qAtan2(opposite, adjacent) * M_180_OVER_PI + 180.0;
  qreal sat = (distance / GetRadius());
  qreal value = hsv_value_;

  return Color::fromHsv(hue, sat, value);
}

bool ColorWheelGLWidget::PointIsValid(const QPoint &p) const
{
  QPoint center = rect().center();
  qreal distance = qSqrt(qPow(p.x() - center.x(), 2) + qPow(p.y() - center.y(), 2));

  return (distance <= GetRadius());
}

OpenGLShader *ColorWheelGLWidget::CreateShader() const
{
  OpenGLShader* s = new OpenGLShader();

  s->create();
  s->addShaderFromSourceCode(QOpenGLShader::Vertex, OpenGLShader::CodeDefaultVertex());
  s->addShaderFromSourceCode(QOpenGLShader::Fragment, Node::ReadFileAsString(QStringLiteral(":/shaders/colorwheel.frag")));
  s->link();

  return s;
}

void ColorWheelGLWidget::SetShaderUniformValues(OpenGLShader *shader) const
{
  shader->setUniformValue("hsv_value", hsv_value_);
}

void ColorWheelGLWidget::resizeEvent(QResizeEvent *e)
{
  ColorSwatchWidget::resizeEvent(e);

  emit DiameterChanged(GetDiameter());
}

int ColorWheelGLWidget::GetDiameter() const
{
  return qMin(width(), height());
}

qreal ColorWheelGLWidget::GetRadius() const
{
  return GetDiameter() * 0.5;
}

/* Software/QPainter-based color wheel
void ColorWheelWidget::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  QPainter p(this);

  qreal radius = qMin(width(), height()) / 2;
  QPoint center = rect().center();

  int x_start = center.x() - radius;
  int x_end = center.x() + radius;
  int y_start = center.y() - radius;
  int y_end = center.y() + radius;

  for (int i=x_start;i<x_end;i++) {
    for (int j=y_start;j<y_end;j++) {
      qreal opposite = j - center.y();
      qreal adjacent = i - center.x();
      qreal pix_len = qSqrt(qPow(adjacent, 2) + qPow(opposite, 2));

      if (pix_len <= radius) {
        qreal hue = qAtan2(opposite, adjacent) * RADIAN_TO_0_1_RANGE + 0.5;
        qreal sat = (pix_len / radius);
        qreal value = 1.0;

        QColor c;
        c.setHsvF(hue, sat, value);
        p.setPen(c);

        p.drawPoint(i, j);
      }
    }
  }
}*/
