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

#include <QDateTime>
#include <QLabel>
#include <QResizeEvent>
#include <QtMath>
#include <QVBoxLayout>

#include "audio/audiomanager.h"
#include "common/timecodefunctions.h"

ViewerWidget::ViewerWidget(QWidget *parent) :
  QWidget(parent),
  viewer_node_(nullptr),
  playback_speed_(0)
{
  // Set up main layout
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);

  // Create main OpenGL-based view
  sizer_ = new ViewerSizer(this);
  layout->addWidget(sizer_);

  gl_widget_ = new ViewerGLWidget(this);
  sizer_->SetWidget(gl_widget_);

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
  connect(controls_, SIGNAL(PlayClicked()), this, SLOT(Play()));
  connect(controls_, SIGNAL(PauseClicked()), this, SLOT(Pause()));
  connect(controls_, SIGNAL(PrevFrameClicked()), this, SLOT(PrevFrame()));
  connect(controls_, SIGNAL(NextFrameClicked()), this, SLOT(NextFrame()));
  connect(controls_, SIGNAL(BeginClicked()), this, SLOT(GoToStart()));
  connect(controls_, SIGNAL(EndClicked()), this, SLOT(GoToEnd()));
  layout->addWidget(controls_);

  // Connect timer
  connect(&playback_timer_, SIGNAL(timeout()), this, SLOT(PlaybackTimerUpdate()));

  // FIXME: Magic number
  ruler_->SetScale(48.0);
}

void ViewerWidget::SetTimebase(const rational &r)
{
  time_base_ = r;
  time_base_dbl_ = r.toDouble();

  ruler_->SetTimebase(r);
  controls_->SetTimebase(r);

  playback_timer_.setInterval(qFloor(r.toDouble()));
}

const double &ViewerWidget::scale()
{
  return ruler_->scale();
}

rational ViewerWidget::GetTime()
{
  return olive::timestamp_to_time(ruler_->GetTime(), time_base_);
}

void ViewerWidget::SetScale(const double &scale_)
{
  ruler_->SetScale(scale_);
}

void ViewerWidget::SetTime(const int64_t &time)
{
  ruler_->SetTime(time);
  UpdateTimeInternal(time);
}

void ViewerWidget::TogglePlayPause()
{
  if (IsPlaying()) {
    Pause();
  } else {
    Play();
  }
}

bool ViewerWidget::IsPlaying()
{
  return playback_timer_.isActive();
}

void ViewerWidget::ConnectViewerNode(ViewerOutput *node)
{
  if (viewer_node_ != nullptr) {
    SetTimebase(0);

    disconnect(viewer_node_, SIGNAL(TimebaseChanged(const rational&)), this, SLOT(SetTimebase(const rational&)));
    disconnect(viewer_node_, SIGNAL(TextureChangedBetween(const rational&, const rational&)), this, SLOT(ViewerNodeChangedBetween(const rational&, const rational&)));
    disconnect(viewer_node_, SIGNAL(SizeChanged(int, int)), this, SLOT(SizeChangedSlot(int, int)));

    // Effectively disables the viewer and clears the state
    SizeChangedSlot(0, 0);
  }

  viewer_node_ = node;

  // Set texture to new texture (or null if no viewer node is available)
  UpdateTextureFromNode(GetTime());

  if (viewer_node_ != nullptr) {
    SetTimebase(viewer_node_->Timebase());

    connect(viewer_node_, SIGNAL(TimebaseChanged(const rational&)), this, SLOT(SetTimebase(const rational&)));
    connect(viewer_node_, SIGNAL(TextureChangedBetween(const rational&, const rational&)), this, SLOT(ViewerNodeChangedBetween(const rational&, const rational&)));
    connect(viewer_node_, SIGNAL(SizeChanged(int, int)), this, SLOT(SizeChangedSlot(int, int)));

    SizeChangedSlot(viewer_node_->ViewerWidth(), viewer_node_->ViewerHeight());
  }
}

void ViewerWidget::DisconnectViewerNode()
{
  ConnectViewerNode(nullptr);
}

void ViewerWidget::SetTexture(RenderTexturePtr tex)
{
  if (tex == nullptr) {
    gl_widget_->SetTexture(0);
  } else {
    gl_widget_->SetTexture(tex->texture());
  }
}

void ViewerWidget::UpdateTimeInternal(int64_t i)
{
  rational time_set = rational(i) * time_base_;

  controls_->SetTime(i);

  if (viewer_node_ != nullptr) {
    UpdateTextureFromNode(time_set);
  }

  emit TimeChanged(i);
}

void ViewerWidget::UpdateTextureFromNode(const rational& time)
{
  if (viewer_node_ == nullptr) {
    SetTexture(nullptr);
  } else {
    SetTexture(viewer_node_->GetTexture(time));
  }
}

void ViewerWidget::PlayInternal(int speed)
{
  Q_ASSERT(speed != 0);

  if (time_base_.isNull()) {
    qWarning() << "ViewerWidget can't play with an invalid timebase";
    return;
  }

  start_msec_ = QDateTime::currentMSecsSinceEpoch();
  start_timestamp_ = ruler_->GetTime();
  playback_speed_ = speed;

  QFile* file = new QFile("/home/matt/Desktop/11 - Santa Monica.raw");
  if (file->open(QFile::ReadOnly)) {
    file->seek(static_cast<qint64>(qFloor(GetTime().toDouble() * 48000 * 2)) * static_cast<qint64>(sizeof(float)));

    AudioManager::instance()->StartOutput(file);
  }

  playback_timer_.start();

  controls_->ShowPauseButton();
}

void ViewerWidget::RulerTimeChange(int64_t i)
{
  Pause();

  UpdateTimeInternal(i);
}

void ViewerWidget::Play()
{
  PlayInternal(1);
}

void ViewerWidget::Pause()
{
  playback_timer_.stop();

  controls_->ShowPlayButton();

  playback_speed_ = 0;

  AudioManager::instance()->StopOutput();
}

void ViewerWidget::GoToStart()
{
  Pause();

  SetTime(0);
}

void ViewerWidget::PrevFrame()
{
  Pause();

  SetTime(qMax(static_cast<int64_t>(0), ruler_->GetTime() - 1));
}

void ViewerWidget::NextFrame()
{
  Pause();

  SetTime(ruler_->GetTime() + 1);
}

void ViewerWidget::GoToEnd()
{
  Pause();

  qWarning() << "No end frame support yet";
}

void ViewerWidget::ShuttleLeft()
{
  int current_speed = playback_speed_;

  if (current_speed != 0) {
    Pause();
  }

  current_speed--;

  if (current_speed != 0) {
    PlayInternal(current_speed);
  }
}

void ViewerWidget::ShuttleStop()
{
  Pause();
}

void ViewerWidget::ShuttleRight()
{
  int current_speed = playback_speed_;

  if (current_speed != 0) {
    Pause();
  }

  current_speed++;

  if (current_speed != 0) {
    PlayInternal(current_speed);
  }
}

void ViewerWidget::PlaybackTimerUpdate()
{
  int64_t real_time = QDateTime::currentMSecsSinceEpoch() - start_msec_;

  int64_t frames_since_start = qRound(static_cast<double>(real_time) / (time_base_dbl_ * 1000));

  int64_t current_time = start_timestamp_ + frames_since_start * playback_speed_;

  if (current_time < 0) {
    current_time = 0;
    Pause();
  }

  SetTime(current_time);
}

void ViewerWidget::ViewerNodeChangedBetween(const rational &start, const rational &end)
{
  if (GetTime() >= start && GetTime() <= end) {
    UpdateTextureFromNode(GetTime());
  }
}

void ViewerWidget::SizeChangedSlot(int width, int height)
{
  sizer_->SetChildSize(width, height);
}

void ViewerWidget::resizeEvent(QResizeEvent *event)
{
  // Set scrollbar page step to the width
  scrollbar_->setPageStep(event->size().width());
}
