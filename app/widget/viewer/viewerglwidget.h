#ifndef VIEWERGLWIDGET_H
#define VIEWERGLWIDGET_H

#include <QOpenGLWidget>

class ViewerGLWidget : public QOpenGLWidget
{
public:
  ViewerGLWidget(QWidget* parent);

  void SetTexture(GLuint tex);
protected:
  virtual void paintGL() override;
private:
  GLuint texture_;
};

#endif // VIEWERGLWIDGET_H
