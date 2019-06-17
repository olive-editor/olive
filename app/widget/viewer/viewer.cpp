#include "viewer.h"

#include <QVBoxLayout>
#include <QLabel>

ViewerWidget::ViewerWidget(QWidget *parent) :
  QWidget(parent)
{
  // Set up main layout
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  // Create main OpenGL-based view
  gl_widget_ = new ViewerGLWidget(this);
  layout->addWidget(gl_widget_);

  // Create lower controls
  controls_ = new PlaybackControls(this);
  controls_->SetTimecodeEnabled(true);
  controls_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  layout->addWidget(controls_);
}
