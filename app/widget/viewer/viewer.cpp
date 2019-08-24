/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "viewer.h"

#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>

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

  // Create time ruler
  ruler_ = new TimeRuler(false, this);
  layout->addWidget(ruler_);
  connect(ruler_, SIGNAL(TimeChanged(int64_t)), this, SLOT(RulerTimeChange(int64_t)));

  // Create scrollbar
  scrollbar_ = new QScrollBar(Qt::Horizontal, this);
  layout->addWidget(scrollbar_);
  connect(scrollbar_, SIGNAL(valueChanged(int)), ruler_, SLOT(SetScroll(int)));
  scrollbar_->setPageStep(ruler_->width());

  // Create lower controls
  controls_ = new PlaybackControls(this);
  controls_->SetTimecodeEnabled(true);
  controls_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  layout->addWidget(controls_);

  // FIXME: Magic number
  ruler_->SetScale(48.0);
}

void ViewerWidget::SetTimebase(const rational &r)
{
  time_base_ = r;

  ruler_->SetTimebase(r);
  controls_->SetTimebase(r);
}

const double &ViewerWidget::scale()
{
  return ruler_->scale();
}

void ViewerWidget::SetScale(const double &scale_)
{
  ruler_->SetScale(scale_);
}

void ViewerWidget::SetTexture(GLuint tex)
{
  gl_widget_->SetTexture(tex);
}

void ViewerWidget::RulerTimeChange(int64_t i)
{
  rational time_set = rational(i) * time_base_;

  controls_->SetTime(i);

  emit TimeChanged(time_set);
}

void ViewerWidget::resizeEvent(QResizeEvent *event)
{
  // Set scrollbar page step to the width
  scrollbar_->setPageStep(event->size().width());
}
