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
#include "project/item/sequence/sequence.h"
#include "project/project.h"
#include "render/pixelservice.h"

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
  ruler_ = new TimeRuler(false, true);
  layout->addWidget(ruler_);
  connect(ruler_, &TimeRuler::TimeChanged, this, &ViewerWidget::RulerTimeChange);

  // Create scrollbar
  scrollbar_ = new QScrollBar(Qt::Horizontal);
  layout->addWidget(scrollbar_);
  connect(scrollbar_, &QScrollBar::valueChanged, ruler_, &TimeRuler::SetScroll);
  scrollbar_->setPageStep(ruler_->width());

  // Create lower controls
  controls_ = new PlaybackControls();
  controls_->SetTimecodeEnabled(true);
  controls_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  connect(controls_, &PlaybackControls::PlayClicked, this, &ViewerWidget::Play);
  connect(controls_, &PlaybackControls::PauseClicked, this, &ViewerWidget::Pause);
  connect(controls_, &PlaybackControls::PrevFrameClicked, this, &ViewerWidget::PrevFrame);
  connect(controls_, &PlaybackControls::NextFrameClicked, this, &ViewerWidget::NextFrame);
  connect(controls_, &PlaybackControls::BeginClicked, this, &ViewerWidget::GoToStart);
  connect(controls_, &PlaybackControls::EndClicked, this, &ViewerWidget::GoToEnd);
  layout->addWidget(controls_);

  // Connect timer
  connect(&playback_timer_, &QTimer::timeout, this, &ViewerWidget::PlaybackTimerUpdate);

  // FIXME: Magic number
  ruler_->SetScale(48.0);

  // Start background renderers
  video_renderer_ = new OpenGLBackend(this);
  connect(video_renderer_, &VideoRenderBackend::CachedFrameReady, this, &ViewerWidget::RendererCachedFrame);
  connect(video_renderer_, &VideoRenderBackend::CachedTimeReady, this, &ViewerWidget::RendererCachedTime);
  connect(video_renderer_, &VideoRenderBackend::CachedTimeReady, ruler_, &TimeRuler::CacheTimeReady);
  connect(video_renderer_, &VideoRenderBackend::RangeInvalidated, ruler_, &TimeRuler::CacheInvalidatedRange);
  audio_renderer_ = new AudioBackend(this);

  connect(PixelService::instance(), &PixelService::FormatChanged, this, &ViewerWidget::UpdateRendererParameters);
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
  return Timecode::timestamp_to_time(ruler_->GetTime(), time_base_);
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

void ViewerWidget::ConnectViewerNode(ViewerOutput *node, ColorManager* color_manager)
{
  if (viewer_node_ != nullptr) {
    SetTimebase(0);

    disconnect(viewer_node_, &ViewerOutput::TimebaseChanged, this, &ViewerWidget::SetTimebase);
    disconnect(viewer_node_, &ViewerOutput::SizeChanged, this, &ViewerWidget::SizeChangedSlot);
    disconnect(viewer_node_, &ViewerOutput::LengthChanged, this, &ViewerWidget::LengthChangedSlot);

    // Effectively disables the viewer and clears the state
    SizeChangedSlot(0, 0);

    gl_widget_->DisconnectColorManager();
  }

  viewer_node_ = node;

  video_renderer_->SetViewerNode(viewer_node_);
  audio_renderer_->SetViewerNode(viewer_node_);

  // Set texture to new texture (or null if no viewer node is available)
  UpdateTextureFromNode(GetTime());

  if (viewer_node_ != nullptr) {
    SetTimebase(viewer_node_->video_params().time_base());

    connect(viewer_node_, &ViewerOutput::TimebaseChanged, this, &ViewerWidget::SetTimebase);
    connect(viewer_node_, &ViewerOutput::SizeChanged, this, &ViewerWidget::SizeChangedSlot);
    connect(viewer_node_, &ViewerOutput::LengthChanged, this, &ViewerWidget::LengthChangedSlot);

    SizeChangedSlot(viewer_node_->video_params().width(), viewer_node_->video_params().height());
    LengthChangedSlot(viewer_node_->Length());

    if (color_manager) {
      gl_widget_->ConnectColorManager(color_manager);
    } else if (viewer_node_->parent()) {
      gl_widget_->ConnectColorManager(static_cast<Sequence*>(viewer_node_->parent())->project()->color_manager());
    } else {
      qWarning() << "Failed to find a suitable color manager for the connected viewer node";
    }

    UpdateRendererParameters();
  }
}

ViewerOutput *ViewerWidget::GetConnectedViewer() const
{
  return viewer_node_;
}

void ViewerWidget::SetColorMenuEnabled(bool enabled)
{
  gl_widget_->SetColorMenuEnabled(enabled);
}

void ViewerWidget::SetOverrideSize(int width, int height)
{
  SizeChangedSlot(width, height);
}

void ViewerWidget::SetMatrix(const QMatrix4x4 &mat)
{
  gl_widget_->SetMatrix(mat);
}

VideoRenderBackend *ViewerWidget::video_renderer() const
{
  return video_renderer_;
}

void ViewerWidget::SetTexture(OpenGLTexturePtr tex)
{
  gl_widget_->SetTexture(tex);
}

void ViewerWidget::UpdateTimeInternal(int64_t i)
{
  rational time_set = rational(i) * time_base_;

  controls_->SetTime(i);

  if (viewer_node_ != nullptr && last_time_ != i) {
    UpdateTextureFromNode(time_set);

    PushScrubbedAudio();
  }

  last_time_ = i;

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

  playback_speed_ = speed;

  QIODevice* audio_src = audio_renderer_->GetAudioPullDevice();
  if (audio_src != nullptr && audio_src->open(QIODevice::ReadOnly)) {
    audio_src->seek(audio_renderer_->params().time_to_bytes(GetTime()));
    AudioManager::instance()->SetOutputParams(audio_renderer_->params());
    AudioManager::instance()->StartOutput(audio_src, playback_speed_);
  }

  start_msec_ = QDateTime::currentMSecsSinceEpoch();
  start_timestamp_ = ruler_->GetTime();

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

void ViewerWidget::UpdateRendererParameters()
{
  if (!viewer_node_) {
    return;
  }

  RenderMode::Mode render_mode = RenderMode::kOffline;

  video_renderer_->SetParameters(VideoRenderingParams(viewer_node_->video_params(),
                                                      PixelService::instance()->GetConfiguredFormatForMode(render_mode),
                                                      render_mode,
                                                      2));
  audio_renderer_->SetParameters(AudioRenderingParams(viewer_node_->audio_params(),
                                                      SampleFormat::GetConfiguredFormatForMode(render_mode)));

  video_renderer_->InvalidateCache(0, viewer_node_->Length());
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

    SetTime(Timecode::time_to_timestamp(viewer_node_->Length(), time_base_));
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

void ViewerWidget::RendererCachedFrame(const rational &time, QVariant value, qint64 job_time)
{
  if (GetTime() == time) {
    frame_cache_job_time_ = job_time;

    SetTexture(value.value<OpenGLTexturePtr>());
  }
}

void ViewerWidget::RendererCachedTime(const rational &time, qint64 job_time)
{
  if (GetTime() == time && job_time > frame_cache_job_time_) {
    frame_cache_job_time_ = job_time;

    UpdateTextureFromNode(GetTime());
  }
}

void ViewerWidget::SizeChangedSlot(int width, int height)
{
  sizer_->SetChildSize(width, height);
}

void ViewerWidget::LengthChangedSlot(const rational &length)
{
  controls_->SetEndTime(Timecode::time_to_timestamp(length, time_base_));
  ruler_->SetCacheStatusLength(length);
}

void ViewerWidget::resizeEvent(QResizeEvent *event)
{
  // Set scrollbar page step to the width
  scrollbar_->setPageStep(event->size().width());
}
