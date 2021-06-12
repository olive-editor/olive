/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include <QGuiApplication>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QResizeEvent>
#include <QScreen>
#include <QtMath>
#include <QVBoxLayout>

#include "audio/audiomanager.h"
#include "common/power.h"
#include "common/ratiodialog.h"
#include "common/timecodefunctions.h"
#include "config/config.h"
#include "node/project/project.h"
#include "render/rendermanager.h"
#include "task/taskmanager.h"
#include "widget/menu/menu.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

#define super TimeBasedWidget

QVector<ViewerWidget*> ViewerWidget::instances_;

const int kMaxPreQueueSize = 8;

ViewerWidget::ViewerWidget(QWidget *parent) :
  super(false, true, parent),
  playback_speed_(0),
  frame_cache_job_time_(0),
  color_menu_enabled_(true),
  time_changed_from_timer_(false),
  prequeuing_(false),
  active_queue_jobs_(0),
  cache_time_(rational::NaN)
{
  // Set up main layout
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);

  // Set up stacked widget to allow switching away from the viewer widget
  stack_ = new QStackedWidget();
  layout->addWidget(stack_);

  // Create main OpenGL-based view and sizer
  sizer_ = new ViewerSizer();
  stack_->addWidget(sizer_);

  display_widget_ = new ViewerDisplayWidget();
  display_widget_->setAcceptDrops(true);
  display_widget_->SetShowWidgetBackground(true);
  connect(display_widget_, &ViewerDisplayWidget::customContextMenuRequested, this, &ViewerWidget::ShowContextMenu);
  connect(display_widget_, &ViewerDisplayWidget::CursorColor, this, &ViewerWidget::CursorColor);
  connect(display_widget_, &ViewerDisplayWidget::ColorProcessorChanged, this, &ViewerWidget::ColorProcessorChanged);
  connect(display_widget_, &ViewerDisplayWidget::ColorManagerChanged, this, &ViewerWidget::ColorManagerChanged);
  connect(display_widget_, &ViewerDisplayWidget::DragEntered, this, &ViewerWidget::DragEntered);
  connect(display_widget_, &ViewerDisplayWidget::Dropped, this, &ViewerWidget::Dropped);
  connect(display_widget_, &ViewerDisplayWidget::VisibilityChanged, this, &ViewerWidget::Pause);
  connect(display_widget_, &ViewerDisplayWidget::TextureChanged, this, &ViewerWidget::TextureChanged);
  connect(sizer_, &ViewerSizer::RequestScale, display_widget_, &ViewerDisplayWidget::SetMatrixZoom);
  connect(sizer_, &ViewerSizer::RequestTranslate, display_widget_, &ViewerDisplayWidget::SetMatrixTranslate);
  connect(display_widget_, &ViewerDisplayWidget::HandDragMoved, sizer_, &ViewerSizer::HandDragMove);
  sizer_->SetWidget(display_widget_);

  // Create waveform view when audio is connected and video isn't
  waveform_view_ = new AudioWaveformView();
  PassWheelEventsToScrollBar(waveform_view_);
  stack_->addWidget(waveform_view_);

  // Create time ruler
  layout->addWidget(ruler());

  // Create scrollbar
  layout->addWidget(scrollbar());
  connect(scrollbar(), &QScrollBar::valueChanged, ruler(), &TimeRuler::SetScroll);
  connect(scrollbar(), &QScrollBar::valueChanged, waveform_view_, &AudioWaveformView::SetScroll);

  // Create lower controls
  controls_ = new PlaybackControls();
  controls_->SetTimecodeEnabled(true);
  controls_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  connect(controls_, &PlaybackControls::PlayClicked, this, static_cast<void(ViewerWidget::*)()>(&ViewerWidget::Play));
  connect(controls_, &PlaybackControls::PauseClicked, this, &ViewerWidget::Pause);
  connect(controls_, &PlaybackControls::PrevFrameClicked, this, &ViewerWidget::PrevFrame);
  connect(controls_, &PlaybackControls::NextFrameClicked, this, &ViewerWidget::NextFrame);
  connect(controls_, &PlaybackControls::BeginClicked, this, &ViewerWidget::GoToStart);
  connect(controls_, &PlaybackControls::EndClicked, this, &ViewerWidget::GoToEnd);
  connect(controls_, &PlaybackControls::TimeChanged, this, &ViewerWidget::SetTimeAndSignal);
  layout->addWidget(controls_);

  // If audio is invalidated during playback, we wait some time before starting it again
  audio_restart_timer_.setInterval(250);
  audio_restart_timer_.setSingleShot(true);
  connect(&audio_restart_timer_, &QTimer::timeout, this, &ViewerWidget::StartAudioOutput);

  // FIXME: Magic number
  SetScale(48.0);

  // Ensures that seeking on the waveform view updates the time as expected
  connect(waveform_view_, &AudioWaveformView::TimeChanged, this, &ViewerWidget::TimeChangedFromWaveform);
  connect(waveform_view_, &AudioWaveformView::customContextMenuRequested, this, &ViewerWidget::ShowContextMenu);

  connect(&playback_backup_timer_, &QTimer::timeout, this, &ViewerWidget::PlaybackTimerUpdate);

  SetAutoMaxScrollBar(true);

  instances_.append(this);

  setAcceptDrops(true);
}

ViewerWidget::~ViewerWidget()
{
  instances_.removeOne(this);

  auto windows = windows_;

  foreach (ViewerWindow* window, windows) {
    delete window;
  }
}

void ViewerWidget::TimeChangedEvent(const int64_t &i)
{
  if (!time_changed_from_timer_) {
    PauseInternal();
  }

  controls_->SetTime(i);

  {
    // Update waveform time
    qint64 waveform_time;

    if (waveform_view_->timebase() != this->timebase()) {
      waveform_time = Timecode::rescale_timestamp(i, this->timebase(), waveform_view_->timebase());
    } else {
      waveform_time = i;
    }

    waveform_view_->SetTime(waveform_time);
  }

  if (GetConnectedNode() && last_time_ != i) {
    rational time_set = Timecode::timestamp_to_time(i, timebase());

    if (!IsPlaying()) {
      UpdateTextureFromNode();

      PushScrubbedAudio();

      // We don't clear the FPS timer on pause in case users want to see it immediately after, but by
      // the time a new texture is drawn, assume that the FPS no longer needs to be shown.
      display_widget_->ResetFPSTimer();
    }

    display_widget_->SetTime(time_set);
  }

  // Send time to auto-cacher
  UpdateAutoCacher();

  last_time_ = i;
}

void ViewerWidget::ConnectNodeEvent(ViewerOutput *n)
{
  connect(n, &ViewerOutput::SizeChanged, this, &ViewerWidget::SetViewerResolution);
  connect(n, &ViewerOutput::PixelAspectChanged, this, &ViewerWidget::SetViewerPixelAspect);
  connect(n, &ViewerOutput::LengthChanged, this, &ViewerWidget::LengthChangedSlot);
  connect(n, &ViewerOutput::InterlacingChanged, this, &ViewerWidget::InterlacingChangedSlot);
  connect(n, &ViewerOutput::VideoParamsChanged, this, &ViewerWidget::UpdateRendererVideoParameters);
  connect(n, &ViewerOutput::AudioParamsChanged, this, &ViewerWidget::UpdateRendererAudioParameters);
  connect(n->video_frame_cache(), &FrameHashCache::Invalidated, this, &ViewerWidget::ViewerInvalidatedVideoRange);
  connect(n->video_frame_cache(), &FrameHashCache::Shifted, this, &ViewerWidget::ViewerShiftedRange);
  connect(n->audio_playback_cache(), &AudioPlaybackCache::Invalidated, this, &ViewerWidget::AudioCacheInvalidated);
  connect(n->audio_playback_cache(), &AudioPlaybackCache::Validated, this, &ViewerWidget::AudioCacheValidated);
  connect(n, &ViewerOutput::TextureInputChanged, this, &ViewerWidget::UpdateStack);

  VideoParams vp = n->GetVideoParams();

  InterlacingChangedSlot(vp.interlacing());

  ruler()->SetPlaybackCache(n->video_frame_cache());

  SetViewerResolution(vp.width(), vp.height());
  SetViewerPixelAspect(vp.pixel_aspect_ratio());
  last_length_ = 0;
  LengthChangedSlot(n->GetLength());

  ColorManager* color_manager = n->project()->color_manager();

  display_widget_->ConnectColorManager(color_manager);
  foreach (ViewerWindow* window, windows_) {
    window->display_widget()->ConnectColorManager(color_manager);
  }

  UpdateStack();

  waveform_view_->SetViewer(GetConnectedNode()->audio_playback_cache());
  waveform_view_->ConnectTimelinePoints(GetConnectedNode()->GetTimelinePoints());

  UpdateRendererVideoParameters();
  UpdateRendererAudioParameters();

  // Set texture to new texture (or null if no viewer node is available)
  UpdateTextureFromNode();
}

void ViewerWidget::DisconnectNodeEvent(ViewerOutput *n)
{
  PauseInternal();

  disconnect(n, &ViewerOutput::SizeChanged, this, &ViewerWidget::SetViewerResolution);
  disconnect(n, &ViewerOutput::PixelAspectChanged, this, &ViewerWidget::SetViewerPixelAspect);
  disconnect(n, &ViewerOutput::LengthChanged, this, &ViewerWidget::LengthChangedSlot);
  disconnect(n, &ViewerOutput::InterlacingChanged, this, &ViewerWidget::InterlacingChangedSlot);
  disconnect(n, &ViewerOutput::VideoParamsChanged, this, &ViewerWidget::UpdateRendererVideoParameters);
  disconnect(n, &ViewerOutput::AudioParamsChanged, this, &ViewerWidget::UpdateRendererAudioParameters);
  disconnect(n->video_frame_cache(), &FrameHashCache::Invalidated, this, &ViewerWidget::ViewerInvalidatedVideoRange);
  disconnect(n->video_frame_cache(), &FrameHashCache::Shifted, this, &ViewerWidget::ViewerShiftedRange);
  disconnect(n->audio_playback_cache(), &AudioPlaybackCache::Invalidated, this, &ViewerWidget::AudioCacheInvalidated);
  disconnect(n->audio_playback_cache(), &AudioPlaybackCache::Validated, this, &ViewerWidget::AudioCacheValidated);
  disconnect(n, &ViewerOutput::TextureInputChanged, this, &ViewerWidget::UpdateStack);

  SetDisplayImage(QVariant());

  ruler()->SetPlaybackCache(nullptr);

  // Effectively disables the viewer and clears the state
  SetViewerResolution(0, 0);

  display_widget_->DisconnectColorManager();
  foreach (ViewerWindow* window, windows_) {
    window->display_widget()->DisconnectColorManager();
  }

  waveform_view_->SetViewer(nullptr);
  waveform_view_->ConnectTimelinePoints(nullptr);

  // Queue an UpdateStack so that when it runs, the viewer node will be fully disconnected
  QMetaObject::invokeMethod(this, "UpdateStack", Qt::QueuedConnection);
}

void ViewerWidget::ConnectedNodeChangeEvent(ViewerOutput *n)
{
  auto_cacher_.SetViewerNode(n);
  cache_time_ = rational::NaN;
}

void ViewerWidget::ScaleChangedEvent(const double &s)
{
  super::ScaleChangedEvent(s);

  waveform_view_->SetScale(s);
}

void ViewerWidget::resizeEvent(QResizeEvent *event)
{
  super::resizeEvent(event);

  UpdateMinimumScale();
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
  return playback_speed_ != 0;
}

void ViewerWidget::SetColorMenuEnabled(bool enabled)
{
  color_menu_enabled_ = enabled;
}

void ViewerWidget::SetMatrix(const QMatrix4x4 &mat)
{
  display_widget_->SetMatrixCrop(mat);
  foreach (ViewerWindow* vw, windows_) {
    vw->display_widget()->SetMatrixCrop(mat);
  }
}

void ViewerWidget::SetFullScreen(QScreen *screen)
{
  if (!screen) {
    // Try to find the screen that contains the mouse cursor currently
    foreach (QScreen* test, QGuiApplication::screens()) {
      if (test->geometry().contains(QCursor::pos())) {
        screen = test;
        break;
      }
    }

    // Fallback, just use the first screen
    if (!screen) {
      screen = QGuiApplication::screens().first();
    }
  }

  if (windows_.contains(screen)) {
    ViewerWindow* vw = windows_.take(screen);
    vw->deleteLater();
    return;
  }

  ViewerWindow* vw = new ViewerWindow(this);

  vw->setGeometry(screen->geometry());
  vw->showFullScreen();
  vw->display_widget()->ConnectColorManager(color_manager());
  connect(vw, &ViewerWindow::destroyed, this, &ViewerWidget::WindowAboutToClose);
  connect(vw->display_widget(), &ViewerDisplayWidget::customContextMenuRequested, this, &ViewerWidget::ShowContextMenu);

  if (GetConnectedNode()) {
    vw->SetVideoParams(GetConnectedNode()->GetVideoParams());
    vw->display_widget()->SetDeinterlacing(vw->display_widget()->IsDeinterlacing());
  }

  vw->display_widget()->SetImage(QVariant::fromValue(display_widget()->GetCurrentTexture()));

  windows_.insert(screen, vw);
}

void ViewerWidget::SetAutoCacheEnabled(bool e)
{
  auto_cacher_.SetPaused(!e);

  if (e) {
    // Enable auto-cache
    UpdateAutoCacher();
  } else {
    // Disable auto-cache
    ClearAutoCacherQueue();
  }
}

void ViewerWidget::CacheEntireSequence()
{
  auto_cacher_.ForceCacheRange(TimeRange(0, GetConnectedNode()->GetVideoLength()));
}

void ViewerWidget::CacheSequenceInOut()
{
  if (GetConnectedNode() && GetConnectedNode()->GetTimelinePoints()->workarea()->enabled()) {
    auto_cacher_.ForceCacheRange(GetConnectedNode()->GetTimelinePoints()->workarea()->range());
  } else {
    QMessageBox::warning(this,
                         tr("Error"),
                         tr("No in or out points are set to cache."),
                         QMessageBox::Ok);
  }
}

void ViewerWidget::SetGizmos(Node *node)
{
  display_widget_->SetTimeTarget(GetConnectedNode());
  display_widget_->SetGizmos(node);
}

FramePtr ViewerWidget::DecodeCachedImage(const QString &cache_path, const QByteArray& hash, const rational& time)
{
  FramePtr frame = FrameHashCache::LoadCacheFrame(cache_path, hash);

  if (frame) {
    frame->set_timestamp(time);
  } else {
    qWarning() << "Tried to load cached frame from file but it was null";
  }

  return frame;
}

void ViewerWidget::DecodeCachedImage(RenderTicketPtr ticket, const QString &cache_path, const QByteArray& hash, const rational& time)
{
  ticket->Start();
  ticket->Finish(QVariant::fromValue(DecodeCachedImage(cache_path, hash, time)));
}

bool ViewerWidget::ShouldForceWaveform() const
{
  return GetConnectedNode()
      && !GetConnectedNode()->GetConnectedTextureOutput().IsValid()
      && GetConnectedNode()->GetConnectedSampleOutput().IsValid();
}

void ViewerWidget::SetEmptyImage()
{
  display_widget()->SetBlank();

  foreach (ViewerWindow *vw, windows_) {
    vw->display_widget()->SetBlank();
  }
}

void ViewerWidget::UpdateAutoCacher()
{
  rational time = GetTime();

  if (GetConnectedNode()              // Ensure valid node
      && cache_time_ != time          // Ensure cache hasn't already been to this time
      && !auto_cacher_.IsPaused()) {  // Follow cache setting
    if (!IsPlaying()) {
      ClearAutoCacherQueue();
    }
    auto_cacher_.SetPlayhead(time);
    cache_time_ = time;
  }
}

void ViewerWidget::ClearAutoCacherQueue()
{
  auto_cacher_.ClearVideoQueue();
  cache_time_ = rational::NaN;
}

void ViewerWidget::StartAudioOutput()
{
  AudioPlaybackCache* audio_cache = GetConnectedNode()->audio_playback_cache();
  if (audio_cache->GetParameters().is_valid()) {
    AudioManager::instance()->SetOutputParams(audio_cache->GetParameters());
    AudioManager::instance()->StartOutput(audio_cache,
                                          audio_cache->GetParameters().time_to_bytes(GetTime()),
                                          playback_speed_);
    emit AudioManager::instance()->OutputWaveformStarted(&audio_cache->visual(),
                                                         GetTime(), playback_speed_);
  }
}

void ViewerWidget::UpdateTextureFromNode()
{
  rational time = GetTime();
  bool frame_exists_at_time = FrameExistsAtTime(time);
  bool frame_might_be_still = ViewerMightBeAStill();

  // Check playback queue for a frame
  if (IsPlaying()) {
    // We still run the playback queue even when FrameExistsAtTime returns false because we might be
    // playing backwards and about to start showing frames, so the queue should be prepared for
    // that.
    bool popped = false;

    if (playback_queue_.empty()) {
      int queue = DeterminePlaybackQueueSize();
      playback_queue_next_frame_ = GetTimestamp() + playback_speed_;
      for (int i=active_queue_jobs_; i<queue; i++) {
        RequestNextFrameForQueue();
      }
    } else {
      while (!playback_queue_.empty()) {

        ViewerPlaybackFrame pf = playback_queue_.front();

        if (pf.timestamp == time) {

          // Frame was in queue, no need to decode anything
          SetDisplayImage(pf.frame, true);
          return;

        } else {

          // Skip this frame
          PopOldestFrameFromPlaybackQueue();
          if (popped) {
            // We've already popped a frame in this loop, meaning a frame has been skipped
            display_widget_->IncrementSkippedFrames();
          } else {
            // Shown a frame and progressed to the next one
            display_widget_->IncrementFrameCount();
            popped = true;
          }

          if (playback_queue_.empty()) {
            // This was the last frame left in the queue. Even though it's old, we'll show it
            // just for improved user feedback
            SetDisplayImage(pf.frame, true);
            return;
          }

        }
      }
    }

    // Only show warning if frame actually exists
    if (frame_exists_at_time && !frame_might_be_still) {
      qWarning() << "Playback queue failed to keep up";
    }

  }

  if (frame_exists_at_time || frame_might_be_still) {
    // Frame was not in queue, will require rendering or decoding from cache
    if (IsPlaying()) {
      // Is playing, yet the queue above failed to retrieve the frame. We effectively do a quick
      // reboot of the queue here, assuming the above loop has emptied it so far.
      display_widget_->update();
    } else {
      // Not playing, run a task to get the frame either from the cache or the renderer
      RenderTicketWatcher* watcher = new RenderTicketWatcher();
      watcher->setProperty("time", QVariant::fromValue(time));
      connect(watcher, &RenderTicketWatcher::Finished, this, &ViewerWidget::RendererGeneratedFrame);
      nonqueue_watchers_.append(watcher);
      watcher->SetTicket(GetFrame(time, true));
    }
  } else {
    // There is definitely no frame here, we can immediately flip to showing nothing
    nonqueue_watchers_.clear();
    SetEmptyImage();
    return;
  }
}

void ViewerWidget::PlayInternal(int speed, bool in_to_out_only)
{
  Q_ASSERT(speed != 0);

  if (!GetConnectedNode()) {
    // Do nothing if no viewer node is attached
    return;
  }

  if (timebase().isNull()) {
    qCritical() << "ViewerWidget can't play with an invalid timebase";
    return;
  }

  // Kindly tell all viewers to stop playing and caching so all resources can be used for playback
  foreach (ViewerWidget* viewer, instances_) {
    if (viewer != this) {
      viewer->PauseInternal();
      viewer->ClearAutoCacherQueue();
    }
  }

  // If the playhead is beyond the end, restart at 0
  if (!in_to_out_only && GetTime() >= GetConnectedNode()->GetLength()) {
    if (speed > 0) {
      SetTimeAndSignal(0);
    } else {
      SetTimeAndSignal(Timecode::time_to_timestamp(GetConnectedNode()->GetLength(), timebase()));
    }
  }

  playback_speed_ = speed;
  play_in_to_out_only_ = in_to_out_only;

  playback_queue_next_frame_ = ruler()->GetTime();

  controls_->ShowPauseButton();

  // Attempt to fill playback queue
  if (display_widget_->isVisible() || !windows_.isEmpty()) {
    prequeue_length_ = DeterminePlaybackQueueSize();

    if (prequeue_length_ > 0) {
      prequeuing_ = true;

      // We "prioritize" the frames, which means they're pushed to the top of the render queue,
      // we queue in reverse so that they're still queued in order

      playback_queue_next_frame_ += playback_speed_ * prequeue_length_;
      int64_t temp = playback_queue_next_frame_;

      for (int i=0; i<prequeue_length_; i++) {
        playback_queue_next_frame_ -= playback_speed_;
        RequestNextFrameForQueue(true, false);
      }

      playback_queue_next_frame_ = temp;
    }
  }

  if (!prequeuing_) {
    FinishPlayPreprocess();
  }
}

void ViewerWidget::PauseInternal()
{
  if (IsPlaying()) {
    AudioManager::instance()->StopOutput();
    playback_speed_ = 0;
    controls_->ShowPlayButton();

    disconnect(display_widget_, &ViewerDisplayWidget::frameSwapped, this, &ViewerWidget::PlaybackTimerUpdate);

    foreach (ViewerWindow* window, windows_) {
      window->Pause();
    }

    playback_queue_.clear();
    playback_backup_timer_.stop();
    audio_restart_timer_.stop();

    UpdateTextureFromNode();
  }

  prequeuing_ = false;
}

void ViewerWidget::PushScrubbedAudio()
{
  if (!IsPlaying() && GetConnectedNode() && Config::Current()["AudioScrubbing"].toBool()) {
    // Get audio src device from renderer
    const AudioParams& params = GetConnectedNode()->audio_playback_cache()->GetParameters();

    if (params.is_valid()) {
      AudioPlaybackCache::PlaybackDevice* audio_src = GetConnectedNode()->audio_playback_cache()->CreatePlaybackDevice();

      if (audio_src->open(QIODevice::ReadOnly)) {
        // FIXME: Hardcoded scrubbing interval (20ms)
        int size_of_sample = params.time_to_bytes(rational(20, 1000));

        // Push audio
        audio_src->seek(params.time_to_bytes(GetTime()));
        QByteArray frame_audio = audio_src->read(size_of_sample);
        AudioManager::instance()->SetOutputParams(params);
        AudioManager::instance()->PushToOutput(frame_audio);

        audio_src->close();
      }

      delete audio_src;
    }
  }
}

void ViewerWidget::UpdateMinimumScale()
{
  if (!GetConnectedNode()) {
    return;
  }

  if (GetConnectedNode()->GetLength().isNull()) {
    // Avoids divide by zero
    SetMinimumScale(0);
  } else {
    SetMinimumScale(static_cast<double>(ruler()->width()) / GetConnectedNode()->GetLength().toDouble());
  }
}

void ViewerWidget::SetColorTransform(const ColorTransform &transform, ViewerDisplayWidget *sender)
{
  sender->SetColorTransform(transform);
}

QString ViewerWidget::GetCachedFilenameFromTime(const rational &time)
{
  if (FrameExistsAtTime(time)) {
    QByteArray hash = GetConnectedNode()->video_frame_cache()->GetHash(time);

    if (!hash.isEmpty()) {
      return GetConnectedNode()->video_frame_cache()->CachePathName(hash);
    }
  }

  return QString();
}

bool ViewerWidget::FrameExistsAtTime(const rational &time)
{
  return GetConnectedNode() && time >= 0 && time < GetConnectedNode()->GetVideoLength();
}

bool ViewerWidget::ViewerMightBeAStill()
{
  return GetConnectedNode() && GetConnectedNode()->GetConnectedTextureOutput().IsValid() && GetConnectedNode()->GetVideoLength().isNull();
}

void ViewerWidget::SetDisplayImage(QVariant frame, bool main_only)
{
  display_widget_->SetImage(frame);

  if (!main_only) {
    foreach (ViewerWindow* vw, windows_) {
      vw->display_widget()->SetImage(frame);
    }
  }
}

void ViewerWidget::RequestNextFrameForQueue(bool prioritize, bool increment)
{
  rational next_time = Timecode::timestamp_to_time(playback_queue_next_frame_,
                                                   timebase());

  if (FrameExistsAtTime(next_time) || ViewerMightBeAStill()) {
    if (increment) {
      playback_queue_next_frame_ += playback_speed_;
    }

    RenderTicketWatcher* watcher = new RenderTicketWatcher();
    watcher->setProperty("time", QVariant::fromValue(next_time));
    connect(watcher, &RenderTicketWatcher::Finished, this, &ViewerWidget::RendererGeneratedFrameForQueue);
    watcher->SetTicket(GetFrame(next_time, prioritize));
    active_queue_jobs_++;
  }
}

RenderTicketPtr ViewerWidget::GetFrame(const rational &t, bool prioritize)
{
  QByteArray cached_hash = GetConnectedNode()->video_frame_cache()->GetHash(t);

  QString cache_fn = GetConnectedNode()->video_frame_cache()->CachePathName(cached_hash);

  if (cached_hash.isEmpty() || !QFileInfo::exists(cache_fn)) {
    // Frame hasn't been cached, start render job
    return auto_cacher_.GetSingleFrame(t, prioritize);
  } else {
    // Frame has been cached, grab the frame
    RenderTicketPtr ticket = std::make_shared<RenderTicket>();
    ticket->setProperty("time", QVariant::fromValue(t));
    QtConcurrent::run(ViewerWidget::DecodeCachedImage, ticket, GetConnectedNode()->video_frame_cache()->GetCacheDirectory(), cached_hash, t);
    return ticket;
  }
}

void ViewerWidget::FinishPlayPreprocess()
{
  int64_t playback_start_time = ruler()->GetTime();

  StartAudioOutput();

  playback_timer_.Start(playback_start_time, playback_speed_, timebase_dbl());
  display_widget_->ResetFPSTimer();

  foreach (ViewerWindow* window, windows_) {
    window->Play(playback_start_time, playback_speed_, timebase());
  }

  if (display_widget_->isVisible()) {
    connect(display_widget_, &ViewerDisplayWidget::frameSwapped, this, &ViewerWidget::PlaybackTimerUpdate);
  } else {
    playback_backup_timer_.setInterval(qFloor(timebase_dbl()));
    playback_backup_timer_.start();
  }

  PlaybackTimerUpdate();
}

int ViewerWidget::DeterminePlaybackQueueSize()
{
  int64_t end_ts;

  if (playback_speed_ > 0) {
    end_ts = Timecode::time_to_timestamp(GetConnectedNode()->GetVideoLength(),
                                         timebase());
  } else {
    end_ts = 0;
  }

  int remaining_frames = (end_ts - GetTimestamp()) / playback_speed_;

  return qMin(kMaxPreQueueSize, remaining_frames);
}

void ViewerWidget::PopOldestFrameFromPlaybackQueue()
{
  playback_queue_.pop_front();

  if (int(playback_queue_.size()) < DeterminePlaybackQueueSize()) {
    RequestNextFrameForQueue();
  }
}

void ViewerWidget::UpdateStack()
{
  rational new_tb;

  if (ShouldForceWaveform()) {
    // If we have a node AND video is disconnected AND audio is connected, show waveform view
    stack_->setCurrentWidget(waveform_view_);
    //new_tb = GetConnectedNode()->audio_params().time_base();
  } else {
    // Otherwise show regular display
    stack_->setCurrentWidget(sizer_);

    /*if (GetConnectedNode()) {
      new_tb = GetConnectedNode()->video_params().time_base();
    }*/
  }

  /*if (new_tb != timebase()) {
    SetTimebase(new_tb);
  }*/
}

void ViewerWidget::ContextMenuSetFullScreen(QAction *action)
{
  SetFullScreen(QGuiApplication::screens().at(action->data().toInt()));
}

void ViewerWidget::ContextMenuDisableSafeMargins()
{
  context_menu_widget_->SetSafeMargins(ViewerSafeMarginInfo(false));
}

void ViewerWidget::ContextMenuSetSafeMargins()
{
  context_menu_widget_->SetSafeMargins(ViewerSafeMarginInfo(true));
}

void ViewerWidget::ContextMenuSetCustomSafeMargins()
{
  bool ok;

  double new_ratio = GetFloatRatioFromUser(this,
                                           tr("Safe Margins"),
                                           &ok);

  if (ok) {
    context_menu_widget_->SetSafeMargins(ViewerSafeMarginInfo(true, new_ratio));
  }
}

void ViewerWidget::WindowAboutToClose()
{
  windows_.remove(windows_.key(static_cast<ViewerWindow*>(sender())));
}

void ViewerWidget::ContextMenuScopeTriggered(QAction *action)
{
  emit RequestScopePanel(static_cast<ScopePanel::Type>(action->data().toInt()));
}

void ViewerWidget::RendererGeneratedFrame()
{
  RenderTicketWatcher* ticket = static_cast<RenderTicketWatcher*>(sender());

  if (ticket->HasResult()) {
    if (nonqueue_watchers_.contains(ticket)) {
      while (!nonqueue_watchers_.isEmpty()) {
        if (nonqueue_watchers_.takeFirst() == ticket) {
          break;
        }
      }

      SetDisplayImage(ticket->Get());
    }
  }

  delete ticket;
}

void ViewerWidget::RendererGeneratedFrameForQueue()
{
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  if (watcher->HasResult()) {
    QVariant frame = watcher->Get();

    // Ignore this signal if we've paused now
    if (IsPlaying() || prequeuing_) {
      rational ts = watcher->property("time").value<rational>();

      playback_queue_.AppendTimewise({ts, frame}, playback_speed_);

      foreach (ViewerWindow* window, windows_) {
        window->queue()->AppendTimewise({ts, frame}, playback_speed_);
      }

      if (prequeuing_ && int(playback_queue_.size()) == prequeue_length_) {
        prequeuing_ = false;
        FinishPlayPreprocess();
      }
    }
  }

  active_queue_jobs_--;

  delete watcher;
}

void ViewerWidget::ShowContextMenu(const QPoint &pos)
{
  if (!GetConnectedNode()) {
    return;
  }

  Menu menu(static_cast<QWidget*>(sender()));

  context_menu_widget_ = dynamic_cast<ViewerDisplayWidget*>(sender());

  // ViewerDisplayWidget options
  if (context_menu_widget_) {
    // Color options
    if (context_menu_widget_->color_manager() && color_menu_enabled_) {
      {
        Menu* ocio_display_menu = context_menu_widget_->GetDisplayMenu(&menu);
        menu.addMenu(ocio_display_menu);
      }

      {
        Menu* ocio_view_menu = context_menu_widget_->GetViewMenu(&menu);
        menu.addMenu(ocio_view_menu);
      }

      {
        Menu* ocio_look_menu = context_menu_widget_->GetLookMenu(&menu);
        menu.addMenu(ocio_look_menu);
      }

      menu.addSeparator();
    }

    {
      // Viewer Zoom Level
      Menu* zoom_menu = new Menu(tr("Zoom"), &menu);
      menu.addMenu(zoom_menu);

      int zoom_levels[] = {10, 25, 50, 75, 100, 150, 200, 400};
      zoom_menu->addAction(tr("Fit"))->setData(0);
      for (int i=0;i<8;i++) {
        zoom_menu->addAction(tr("%1%").arg(zoom_levels[i]))->setData(zoom_levels[i]);
      }

      connect(zoom_menu, &QMenu::triggered, this, &ViewerWidget::SetZoomFromMenu);
    }

    {
      // Full Screen Menu
      Menu* full_screen_menu = new Menu(tr("Full Screen"), &menu);
      menu.addMenu(full_screen_menu);

      for (int i=0;i<QGuiApplication::screens().size();i++) {
        QScreen* s = QGuiApplication::screens().at(i);

        QAction* a = full_screen_menu->addAction(tr("Screen %1: %2x%3").arg(QString::number(i),
                                                                            QString::number(s->size().width()),
                                                                            QString::number(s->size().height())));

        a->setData(i);
        a->setCheckable(true);
        a->setChecked(windows_.contains(QGuiApplication::screens().at(i)));
      }

      connect(full_screen_menu, &QMenu::triggered, this, &ViewerWidget::ContextMenuSetFullScreen);
    }

    {
      // Deinterlace Option
      if (GetConnectedNode()->GetVideoParams().interlacing() != VideoParams::kInterlaceNone) {
        QAction* deinterlace_action = menu.addAction(tr("Deinterlace"));
        deinterlace_action->setCheckable(true);
        deinterlace_action->setChecked(display_widget_->IsDeinterlacing());
        connect(deinterlace_action, &QAction::triggered, display_widget_, &ViewerDisplayWidget::SetDeinterlacing);
      }
    }

    menu.addSeparator();

    {
      // Scopes
      Menu* scopes_menu = new Menu(tr("Scopes"), &menu);
      menu.addMenu(scopes_menu);

      for (int i=0;i<ScopePanel::kTypeCount;i++) {
        QAction* scope_action = scopes_menu->addAction(ScopePanel::TypeToName(static_cast<ScopePanel::Type>(i)));
        scope_action->setData(i);
      }

      connect(scopes_menu, &Menu::triggered, this, &ViewerWidget::ContextMenuScopeTriggered);
    }

    menu.addSeparator();

    {
      Menu* cache_menu = new Menu(tr("Cache"), &menu);
      menu.addMenu(cache_menu);

      // Auto-cache
      QAction* autocache_action = cache_menu->addAction(tr("Auto-Cache"));
      autocache_action->setCheckable(true);
      autocache_action->setChecked(!auto_cacher_.IsPaused());
      connect(autocache_action, &QAction::triggered, this, &ViewerWidget::SetAutoCacheEnabled);

      cache_menu->addSeparator();

      // Cache Entire Sequence
      QAction* cache_entire_sequence = cache_menu->addAction(tr("Cache Entire Sequence"));
      connect(cache_entire_sequence, &QAction::triggered, this, &ViewerWidget::CacheEntireSequence);

      // Cache In/Out Sequence
      QAction* cache_inout_sequence = cache_menu->addAction(tr("Cache Sequence In/Out"));
      connect(cache_inout_sequence, &QAction::triggered, this, &ViewerWidget::CacheSequenceInOut);
    }

    menu.addSeparator();

    {
      // Safe Margins
      Menu* safe_margin_menu = new Menu(tr("Safe Margins"), &menu);
      menu.addMenu(safe_margin_menu);

      QAction* safe_margin_off = safe_margin_menu->addAction(tr("Off"));
      safe_margin_off->setCheckable(true);
      safe_margin_off->setChecked(!context_menu_widget_->GetSafeMargin().is_enabled());
      connect(safe_margin_off, &QAction::triggered, this, &ViewerWidget::ContextMenuDisableSafeMargins);

      QAction* safe_margin_on = safe_margin_menu->addAction(tr("On"));
      safe_margin_on->setCheckable(true);
      safe_margin_on->setChecked(context_menu_widget_->GetSafeMargin().is_enabled() && !context_menu_widget_->GetSafeMargin().custom_ratio());
      connect(safe_margin_on, &QAction::triggered, this, &ViewerWidget::ContextMenuSetSafeMargins);

      QAction* safe_margin_custom = safe_margin_menu->addAction(tr("Custom Aspect"));
      safe_margin_custom->setCheckable(true);
      safe_margin_custom->setChecked(context_menu_widget_->GetSafeMargin().is_enabled() && context_menu_widget_->GetSafeMargin().custom_ratio());
      connect(safe_margin_custom, &QAction::triggered, this, &ViewerWidget::ContextMenuSetCustomSafeMargins);
    }

    menu.addSeparator();
  }

  {
    QAction* show_waveform_action = menu.addAction(tr("Show Audio Waveform"));
    show_waveform_action->setCheckable(true);
    show_waveform_action->setChecked(stack_->currentWidget() == waveform_view_);
    show_waveform_action->setEnabled(!ShouldForceWaveform());
    connect(show_waveform_action, &QAction::triggered, this, &ViewerWidget::ManualSwitchToWaveform);
  }

  {
    QAction* show_fps_action = menu.addAction(tr("Show FPS"));
    show_fps_action->setCheckable(true);
    show_fps_action->setChecked(display_widget_->GetShowFPS());
    connect(show_fps_action, &QAction::triggered, display_widget_, &ViewerDisplayWidget::SetShowFPS);
  }

  menu.exec(static_cast<QWidget*>(sender())->mapToGlobal(pos));
}

void ViewerWidget::Play(bool in_to_out_only)
{
  if (in_to_out_only) {
    if (GetConnectedNode()
        && GetConnectedNode()->GetTimelinePoints()->workarea()->enabled()) {
      // Jump to in point
      SetTimeAndSignal(Timecode::time_to_timestamp(GetConnectedNode()->GetTimelinePoints()->workarea()->in(), timebase()));
    } else {
      in_to_out_only = false;
    }
  }

  PlayInternal(1, in_to_out_only);
}

void ViewerWidget::Play()
{
  Play(false);
}

void ViewerWidget::Pause()
{
  PauseInternal();

  UpdateAutoCacher();
}

void ViewerWidget::ShuttleLeft()
{
  int current_speed = playback_speed_;

  if (current_speed != 0) {
    PauseInternal();
  }

  current_speed--;

  if (current_speed == 0) {
    current_speed--;
  }

  PlayInternal(current_speed, false);
}

void ViewerWidget::ShuttleStop()
{
  Pause();
}

void ViewerWidget::ShuttleRight()
{
  int current_speed = playback_speed_;

  if (current_speed != 0) {
    PauseInternal();
  }

  current_speed++;

  if (current_speed == 0) {
    current_speed++;
  }

  PlayInternal(current_speed, false);
}

void ViewerWidget::SetColorTransform(const ColorTransform &transform)
{
  SetColorTransform(transform, display_widget_);
}

void ViewerWidget::SetSignalCursorColorEnabled(bool e)
{
  display_widget_->SetSignalCursorColorEnabled(e);

  foreach (ViewerWindow* vw, windows_) {
    vw->display_widget()->SetSignalCursorColorEnabled(e);
  }
}

void ViewerWidget::TimebaseChangedEvent(const rational &timebase)
{
  super::TimebaseChangedEvent(timebase);

  controls_->SetTimebase(timebase);

  controls_->SetTime(ruler()->GetTime());
  LengthChangedSlot(GetConnectedNode() ? GetConnectedNode()->GetLength() : 0);
}

void ViewerWidget::PlaybackTimerUpdate()
{
  int64_t current_time = playback_timer_.GetTimestampNow();

  int64_t min_time, max_time;

  {
    if ((play_in_to_out_only_ || Config::Current()["Loop"].toBool())
        && GetConnectedNode()->GetTimelinePoints()->workarea()->enabled()) {

      // If "play in to out" is enabled or we're looping AND we have a workarea, only play the workarea
      min_time = Timecode::time_to_timestamp(GetConnectedNode()->GetTimelinePoints()->workarea()->in(), timebase());
      max_time = Timecode::time_to_timestamp(GetConnectedNode()->GetTimelinePoints()->workarea()->out(), timebase());

    } else {

      // Otherwise set the bounds to the range of the sequence
      min_time = 0;
      max_time = Timecode::time_to_timestamp(GetConnectedNode()->GetLength(), timebase());

    }
  }

  if ((playback_speed_ < 0 && current_time <= min_time)
      || (playback_speed_ > 0 && current_time >= max_time)) {

    // Determine which timestamp we tripped
    int64_t tripped_time;

    if (current_time <= min_time) {
      tripped_time = min_time;
    } else {
      tripped_time = max_time;
    }

    if (Config::Current()[QStringLiteral("Loop")].toBool()) {

      // If we're looping, jump to the other side of the workarea and continue
      int64_t opposing_time = (tripped_time == min_time) ? max_time : min_time;

      // Cache the current speed
      int current_speed = playback_speed_;

      // Jump to the other side and keep playing at the same speed
      SetTimeAndSignal(opposing_time);
      PlayInternal(current_speed, play_in_to_out_only_);

    } else {

      // Pause at the boundary
      SetTimeAndSignal(tripped_time);

    }

  } else {

    // Sets time, wrapping in this bool ensures we don't pause from setting the time
    time_changed_from_timer_ = true;
    SetTimeAndSignal(current_time);
    time_changed_from_timer_ = false;

  }

  if (display_widget_->isVisible()) {
    // Updating display widget
    UpdateTextureFromNode();
  } else if (!windows_.empty()) {
    // We still run the queue if windows are visible even if our own display widget isn't visible
    rational t = GetTime();
    while (!playback_queue_.empty() && playback_queue_.front().timestamp != t) {
      PopOldestFrameFromPlaybackQueue();
    }
  }
}

void ViewerWidget::SetViewerResolution(int width, int height)
{
  sizer_->SetChildSize(width, height);

  foreach (ViewerWindow* vw, windows_) {
    vw->SetResolution(width, height);
  }
}

void ViewerWidget::SetViewerPixelAspect(const rational &ratio)
{
  sizer_->SetPixelAspectRatio(ratio);

  foreach (ViewerWindow* vw, windows_) {
    vw->SetPixelAspectRatio(ratio);
  }
}

void ViewerWidget::LengthChangedSlot(const rational &length)
{
  if (last_length_ != length) {
    controls_->SetEndTime(Timecode::time_to_timestamp(length, timebase()));
    UpdateMinimumScale();

    if (length < last_length_ && GetTime() >= length) {
      UpdateTextureFromNode();
    }

    last_length_ = length;
  }
}

void ViewerWidget::InterlacingChangedSlot(VideoParams::Interlacing interlacing)
{
  // Automatically set a "sane" deinterlacing option
  display_widget_->SetDeinterlacing(interlacing != VideoParams::kInterlaceNone);

  foreach (ViewerWindow* vw, windows_) {
    vw->display_widget()->SetDeinterlacing(interlacing != VideoParams::kInterlaceNone);
  }
}

void ViewerWidget::UpdateRendererVideoParameters()
{
  VideoParams vp = GetConnectedNode()->GetVideoParams();

  display_widget_->SetVideoParams(vp);
  foreach (ViewerWindow* window, windows_) {
    window->display_widget()->SetVideoParams(vp);
  }
}

void ViewerWidget::UpdateRendererAudioParameters()
{
}

void ViewerWidget::SetZoomFromMenu(QAction *action)
{
  sizer_->SetZoom(action->data().toInt());
}

void ViewerWidget::ViewerInvalidatedVideoRange(const TimeRange &range)
{
  // If our current frame is within this range, we need to update
  if (GetTime() >= range.in() && (GetTime() < range.out() || range.in() == range.out())) {
    QMetaObject::invokeMethod(this, "UpdateTextureFromNode", Qt::QueuedConnection);
  }
}

void ViewerWidget::ManualSwitchToWaveform(bool e)
{
  if (e) {
    stack_->setCurrentWidget(waveform_view_);
  } else {
    stack_->setCurrentWidget(sizer_);
  }
}

void ViewerWidget::TimeChangedFromWaveform(qint64 t)
{
  if (waveform_view_->timebase() != this->timebase()) {
    // Transform time to our timebase
    t = Timecode::rescale_timestamp(t, waveform_view_->timebase(), this->timebase());
  }

  SetTimeAndSignal(t);
}

void ViewerWidget::ViewerShiftedRange(const rational &from, const rational &to)
{
  if (GetTime() >= qMin(from, to)) {
    QMetaObject::invokeMethod(this, "UpdateTextureFromNode", Qt::QueuedConnection);
  }
}

void ViewerWidget::DragEntered(QDragEnterEvent* event)
{
  if (event->mimeData()->formats().contains(QStringLiteral("application/x-oliveprojectitemdata"))) {
    event->accept();
  }
}

void ViewerWidget::Dropped(QDropEvent *event)
{
  QByteArray mimedata = event->mimeData()->data(QStringLiteral("application/x-oliveprojectitemdata"));
  QDataStream stream(&mimedata, QIODevice::ReadOnly);

  // Variables to deserialize into
  quintptr item_ptr = 0;
  QVector<Track::Reference> enabled_streams;

  while (!stream.atEnd()) {
    stream >> enabled_streams >> item_ptr;

    // We only need the one item
    break;
  }

  if (item_ptr) {
    Node* item = reinterpret_cast<Node*>(item_ptr);
    ViewerOutput* viewer = dynamic_cast<ViewerOutput*>(item);

    if (viewer) {
      ConnectViewerNode(viewer);
    }
  }
}

void ViewerWidget::AudioCacheInvalidated()
{
  if (IsPlaying()) {
    AudioManager::instance()->StopOutput();
  }
}

void ViewerWidget::AudioCacheValidated()
{
  if (IsPlaying()) {
    // This timer will restart audio
    AudioManager::instance()->StopOutput();
    audio_restart_timer_.stop();
    audio_restart_timer_.start();
  }
}

}
