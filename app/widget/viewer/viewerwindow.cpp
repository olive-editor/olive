#include "viewerwindow.h"

#include <QKeyEvent>
#include <QVBoxLayout>

ViewerWindow::ViewerWindow(QWidget *parent) :
  QWidget(parent, Qt::Window | Qt::WindowStaysOnTopHint)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  gl_widget_ = new ViewerGLWidget();
  layout->addWidget(gl_widget_);
}

ViewerGLWidget *ViewerWindow::gl_widget() const
{
  return gl_widget_;
}

void ViewerWindow::SetResolution(int width, int height)
{
  // Set GL widget matrix to maintain this texture's aspect ratio
  double window_ar = static_cast<double>(this->width()) / static_cast<double>(this->height());
  double image_ar = static_cast<double>(width) / static_cast<double>(height);

  QMatrix4x4 mat;

  if (window_ar > image_ar) {
    // Window is wider than image, adjust X scale
    mat.scale(image_ar / window_ar, 1.0f, 1.0f);
  } else if (window_ar < image_ar) {
    // Window is taller than image, adjust Y scale
    mat.scale(1.0f, window_ar / image_ar, 1.0f);
  }

  gl_widget_->SetMatrix(mat);
}

void ViewerWindow::keyPressEvent(QKeyEvent *e)
{
  QWidget::keyPressEvent(e);

  if (e->key() == Qt::Key_Escape) {
    close();
  }
}

void ViewerWindow::closeEvent(QCloseEvent *e)
{
  QWidget::closeEvent(e);

  deleteLater();
}
