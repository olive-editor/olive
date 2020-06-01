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

#include "viewerwindow.h"

#include <QKeyEvent>
#include <QVBoxLayout>

#include "common/timecodefunctions.h"

OLIVE_NAMESPACE_ENTER

ViewerWindow::ViewerWindow(QWidget *parent) :
  QWidget(parent, Qt::Window | Qt::WindowStaysOnTopHint)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  display_widget_ = new ViewerDisplayWidget();
  layout->addWidget(display_widget_);
}

ViewerDisplayWidget *ViewerWindow::display_widget() const
{
  return display_widget_;
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

  display_widget_->SetMatrixZoom(mat);
}

void ViewerWindow::Play(const int64_t& start_timestamp, const int& playback_speed, const rational &timebase)
{
  timer_.Start(start_timestamp, playback_speed, timebase.toDouble());

  playback_timebase_ = timebase;

  connect(display_widget_, &ViewerDisplayWidget::frameSwapped, this, &ViewerWindow::UpdateFromQueue);

  display_widget_->update();
}

void ViewerWindow::Pause()
{
  disconnect(display_widget_, &ViewerDisplayWidget::frameSwapped, this, &ViewerWindow::UpdateFromQueue);

  queue_.clear();
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

void ViewerWindow::UpdateFromQueue()
{
  int64_t t = timer_.GetTimestampNow();

  rational time = Timecode::timestamp_to_time(t, playback_timebase_);

  while (!queue_.empty()) {
    const ViewerPlaybackFrame& pf = queue_.front();

    if (pf.timestamp == time) {
      // Frame was in queue, no need to decode anything
      display_widget_->SetImage(pf.frame);
      return;
    } else {
      queue_.pop_front();
    }
  }
}

OLIVE_NAMESPACE_EXIT
