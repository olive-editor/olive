#include "colorswatchwidget.h"

#include <QMouseEvent>

#include "render/backend/opengl/openglrenderfunctions.h"

ColorSwatchWidget::ColorSwatchWidget(QWidget *parent) :
  QOpenGLWidget(parent),
  shader_(nullptr)
{
  // Render with transparent background
  setAttribute(Qt::WA_AlwaysStackOnTop);
}

ColorSwatchWidget::~ColorSwatchWidget()
{
  CleanUp();
}

void ColorSwatchWidget::initializeGL()
{
  CleanUp();

  shader_ = CreateShader();

  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ColorSwatchWidget::CleanUp);
}

void ColorSwatchWidget::paintGL()
{
  shader_->bind();
  shader_->setUniformValue("ove_resolution", width(), height());
  SetShaderUniformValues(shader_);
  shader_->release();

  OpenGLRenderFunctions::Blit(shader_);
}

void ColorSwatchWidget::CleanUp()
{
  delete shader_;
  shader_ = nullptr;
}

void ColorSwatchWidget::mousePressEvent(QMouseEvent *e)
{
  QOpenGLWidget::mousePressEvent(e);

  if (PointIsValid(e->pos())) {
    emit ColorChanged(GetColorFromCursorPos(e->pos()));
  }
}

void ColorSwatchWidget::mouseMoveEvent(QMouseEvent *e)
{
  QOpenGLWidget::mouseMoveEvent(e);

  if (e->buttons() & Qt::LeftButton && PointIsValid(e->pos())) {
    emit ColorChanged(GetColorFromCursorPos(e->pos()));
  }
}
