#include "viewerglwidget.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>

#include "render/gl/glfunc.h"
#include "render/gl/shadergenerators.h"

ViewerGLWidget::ViewerGLWidget(QWidget *parent) :
  QOpenGLWidget(parent)
{
}

void ViewerGLWidget::paintGL()
{
  /*
  QOpenGLFunctions* f = context()->functions();

  f->glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
  f->glClear(GL_COLOR_BUFFER_BIT);

  QOpenGLTexture* tex = new QOpenGLTexture(img);

  ShaderPtr pipeline = olive::gl::GetDefaultPipeline();

  tex->bind();

  olive::gl::Blit(pipeline, true);

  tex->release();

  delete tex;
  */
}
