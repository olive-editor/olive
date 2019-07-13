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

  // FIXME: Test code
  connect(controls_, SIGNAL(PlayClicked()), this, SLOT(TemporaryTestFunction()));
  // End test code
}

void ViewerWidget::SetTexture(GLuint tex)
{
  gl_widget_->SetTexture(tex);
}

void ViewerWidget::TemporaryTestFunction()
{
  emit TimeChanged(69);
}
