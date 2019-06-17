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
  gl_widget_ = new ViewerGLWidget();
  layout->addWidget(gl_widget_);

  // Create lower controls
  QWidget* lower_control_container = new QWidget();
  lower_control_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  QHBoxLayout* lower_control_layout = new QHBoxLayout(lower_control_container);
  lower_control_layout->setSpacing(0);
  lower_control_layout->setMargin(0);
  layout->addWidget(lower_control_container);

  QSizePolicy lower_container_size_policy = QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

  // In the lower-left, we create a current timecode label wrapped in a QWidget for fixed sizing
  QWidget* lower_left_container = new QWidget();
  lower_left_container->setSizePolicy(lower_container_size_policy);
  lower_control_layout->addWidget(lower_left_container);

  QHBoxLayout* lower_left_layout = new QHBoxLayout(lower_left_container);
  lower_left_layout->setSpacing(0);
  lower_left_layout->setMargin(0);

  cur_tc_lbl_ = new QLabel("00:00:00;00");
  lower_left_layout->addWidget(cur_tc_lbl_);
  lower_left_layout->addStretch();

  // In the lower-middle, we create playback control buttons
  QWidget* lower_middle_container = new QWidget();
  lower_left_container->setSizePolicy(lower_container_size_policy);
  lower_control_layout->addWidget(lower_middle_container);

  QHBoxLayout* lower_middle_layout = new QHBoxLayout(lower_middle_container);
  lower_middle_layout->setSpacing(0);
  lower_middle_layout->setMargin(0);

  lower_middle_layout->addStretch();

  QPushButton* play_button = new QPushButton("Play");
  lower_middle_layout->addWidget(play_button);

  lower_middle_layout->addStretch();

  // The lower-right, we create another timecode label, this time to show the end timecode
  QWidget* lower_right_container = new QWidget();
  lower_left_container->setSizePolicy(lower_container_size_policy);
  lower_control_layout->addWidget(lower_right_container);

  QHBoxLayout* lower_right_layout = new QHBoxLayout(lower_right_container);
  lower_right_layout->setSpacing(0);
  lower_right_layout->setMargin(0);

  lower_right_layout->addStretch();
  end_tc_lbl_ = new QLabel("00:00:00;00");
  lower_right_layout->addWidget(end_tc_lbl_);
}
