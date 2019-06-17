#include "viewerglwidget.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

ViewerGLWidget::ViewerGLWidget()
{

}

void ViewerGLWidget::paintGL()
{
  QOpenGLFunctions* f = context()->functions();

  f->glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
  f->glClear(GL_COLOR_BUFFER_BIT);
}
