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
#include "config/config.h"

ViewerWidget::ViewerWidget(QWidget *parent) :
  QWidget(parent),
  viewer_node_(nullptr),
  playback_speed_(0)
{
  // Set up main layout
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);

  // Create main OpenGL-based view
  sizer_ = new ViewerSizer();
  layout->addWidget(sizer_);

  gl_widget_ = new ViewerGLWidget();
  sizer_->SetWidget(gl_widget_);

  // Create time ruler
  ruler_ = new TimeRuler(false);
  layout->addWidget(ruler_);
  connect(ruler_, SIGNAL(TimeChanged(int64_t)), this, SLOT(RulerTimeChange(int64_t)));

  // Create scrollbar
  scrollbar_ = new QScrollBar(Qt::Horizontal);
  layout->addWidget(scrollbar_);
  connect(scrollbar_, SIGNAL(valueChanged(int)), ruler_, SLOT(SetScroll(int)));
  scrollbar_->setPageStep(ruler_->width());

  // Create lower controls
  controls_ = new PlaybackControls();
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

  // Start background renderers
  video_renderer_ = new OpenGLBackend(this);
  connect(video_renderer_, SIGNAL(CachedFrameReady(const rational&, QVariant)), this, SLOT(RendererCachedFrame(const rational&, QVariant)));
  connect(video_renderer_, SIGNAL(CachedTimeReady(const rational&)), this, SLOT(RendererCachedTime(const rational&)));
  audio_renderer_ = new AudioBackend(this);
}

void ViewerWidget::SetTimebase(const rational &r)
{
  time_base_ = r;
  time_base_dbl_ = r.toDouble();

  ruler_->SetTimebase(r);
  controls_->SetTimebase(r);

  controls_->SetTime(ruler_->GetTime());
  LengthChangedSlot(viewer_node_ ? viewer_node_->Length() : 0);

  playback_timer_.setInterval(qFloor(r.toDouble()));
}

const double &ViewerWidget::scale() const
{
  return ruler_->scale();
}

rational ViewerWidget::GetTime() const
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

bool ViewerWidget::IsPlaying() const
{
  return playback_timer_.isActive();
}

void ViewerWidget::ConnectViewerNode(ViewerOutput *node)
{
  if (viewer_node_ != nullptr) {
    SetTimebase(0);

    disconnect(viewer_node_, SIGNAL(TimebaseChanged(const rational&)), this, SLOT(SetTimebase(const rational&)));
    disconnect(viewer_node_, SIGNAL(SizeChanged(int, int)), this, SLOT(SizeChangedSlot(int, int)));
    disconnect(viewer_node_, SIGNAL(LengthChanged(const rational&)), this, SLOT(LengthChangedSlot(const rational&)));

    // Effectively disables the viewer and clears the state
    SizeChangedSlot(0, 0);
  }

  viewer_node_ = node;

  // Set texture to new texture (or null if no viewer node is available)
  UpdateTextureFromNode(GetTime());

  if (viewer_node_ != nullptr) {
    SetTimebase(viewer_node_->video_params().time_base());

    connect(viewer_node_, SIGNAL(TimebaseChanged(const rational&)), this, SLOT(SetTimebase(const rational&)));
    connect(viewer_node_, SIGNAL(SizeChanged(int, int)), this, SLOT(SizeChangedSlot(int, int)));
    connect(viewer_node_, SIGNAL(LengthChanged(const rational&)), this, SLOT(LengthChangedSlot(const rational&)));

    SizeChangedSlot(viewer_node_->video_params().width(), viewer_node_->video_params().height());
    LengthChangedSlot(viewer_node_->Length());
  }

  video_renderer_->SetViewerNode(viewer_node_);
  audio_renderer_->SetViewerNode(viewer_node_);
}

void ViewerWidget::DisconnectViewerNode()
{
  ConnectViewerNode(nullptr);
}

ViewerOutput *ViewerWidget::GetConnectedViewer() const
{
  return viewer_node_;
}

void ViewerWidget::SetColorMenuEnabled(bool enabled)
{
  gl_widget_->SetColorMenuEnabled(enabled);
}

void ViewerWidget::SetTexture(OpenGLTexturePtr tex)
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

    PushScrubbedAudio();
  }

  emit TimeChanged(i);
}

void ViewerWidget::UpdateTextureFromNode(const rational& time)
{
  if (viewer_node_ == nullptr) {
    SetTexture(nullptr);
  } else {
    SetTexture(video_renderer_->GetCachedFrameAsTexture(time));
  }
}

void ViewerWidget::PlayInternal(int speed)
{
  Q_ASSERT(speed != 0);

  if (time_base_.isNull()) {
    qWarning() << "ViewerWidget can't play with an invalid timebase";
    return;
  }

  QIODevice* audio_src = audio_renderer_->GetAudioPullDevice();
  if (audio_src != nullptr && audio_src->open(QIODevice::ReadOnly)) {
    audio_src->seek(audio_renderer_->params().time_to_bytes(GetTime()));
    AudioManager::instance()->SetOutputParams(audio_renderer_->params());
    AudioManager::instance()->StartOutput(audio_src);
  }

  start_msec_ = QDateTime::currentMSecsSinceEpoch();
  start_timestamp_ = ruler_->GetTime();
  playback_speed_ = speed;

  playback_timer_.start();

  controls_->ShowPauseButton();
}

void ViewerWidget::PushScrubbedAudio()
{
  if (Config::Current()["AudioScrubbing"].toBool() && !IsPlaying()) {
    // Get audio src device from renderer
    QIODevice* audio_src = audio_renderer_->GetAudioPullDevice();

    if (audio_src != nullptr && audio_src->open(QFile::ReadOnly)) {
      // Try to get one "frame" of audio
      int size_of_sample = audio_renderer_->params().time_to_bytes(time_base_);

      // Push audio
      audio_src->seek(audio_renderer_->params().time_to_bytes(GetTime()));
      QByteArray frame_audio = audio_src->read(size_of_sample);
      AudioManager::instance()->SetOutputParams(audio_renderer_->params());
      AudioManager::instance()->PushToOutput(frame_audio);

      audio_src->close();
    }
  }
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
  if (IsPlaying()) {
    AudioManager::instance()->StopOutput();
    playback_speed_ = 0;
    controls_->ShowPlayButton();
    playback_timer_.stop();
  }
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
  if (viewer_node_ != nullptr) {
    Pause();

    SetTime(olive::time_to_timestamp(viewer_node_->Length(), time_base_));
  }
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

void ViewerWidget::SetOCIOParameters(const QString &display, const QString &view, const QString &look)
{
  gl_widget_->SetOCIOParameters(display, view, look);
}

void ViewerWidget::SetOCIODisplay(const QString &display)
{
  gl_widget_->SetOCIODisplay(display);
}

void ViewerWidget::SetOCIOView(const QString &view)
{
  gl_widget_->SetOCIOView(view);
}

void ViewerWidget::SetOCIOLook(const QString &look)
{
  gl_widget_->SetOCIOLook(look);
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

void ViewerWidget::RendererCachedFrame(const rational &time, QVariant value)
{
  if (GetTime() == time) {
    SetTexture(value.value<OpenGLTexturePtr>());
  }
}

void ViewerWidget::RendererCachedTime(const rational &time)
{
  if (GetTime() == time) {
    UpdateTextureFromNode(GetTime());
  }
}

void ViewerWidget::SizeChangedSlot(int width, int height)
{
  sizer_->SetChildSize(width, height);
}

void ViewerWidget::LengthChangedSlot(const rational &length)
{
  controls_->SetEndTime(olive::time_to_timestamp(length, time_base_));
}

void ViewerWidget::resizeEvent(QResizeEvent *event)
{
  // Set scrollbar page step to the width
  scrollbar_->setPageStep(event->size().width());
}
