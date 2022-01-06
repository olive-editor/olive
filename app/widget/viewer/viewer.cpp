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
#include "common/clamp.h"
#include "common/power.h"
#include "common/ratiodialog.h"
#include "common/timecodefunctions.h"
#include "config/config.h"
#include "core.h"
#include "node/project/project.h"
#include "render/rendermanager.h"
#include "task/taskmanager.h"
#include "viewerpreventsleep.h"
#include "widget/menu/menu.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

#define super TimeBasedWidget

QVector<ViewerWidget*> ViewerWidget::instances_;

// NOTE: Hardcoded interval of size of audio chunk to render and send to the output at a time.
//       We want this to be as long as possible so the code has plenty of time to send the audio
//       while also being as short as possible so users get relatively immediate feedback when
//       changing values. 1/4 second seems to be a good middleground.
const rational ViewerWidget::kAudioPlaybackInterval = rational(1, 4);

const int kMaxPreQueueSize = 8;

ViewerWidget::ViewerWidget(QWidget *parent) :
  super(false, true, parent),
  playback_speed_(0),
  color_menu_enabled_(true),
  time_changed_from_timer_(false),
  prequeuing_video_(false),
  prequeuing_audio_(0)
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
  playback_devices_.append(display_widget_);
  connect(display_widget_, &ViewerDisplayWidget::customContextMenuRequested, this, &ViewerWidget::ShowContextMenu);
  connect(display_widget_, &ViewerDisplayWidget::CursorColor, this, &ViewerWidget::CursorColor);
  connect(display_widget_, &ViewerDisplayWidget::ColorProcessorChanged, this, &ViewerWidget::ColorProcessorChanged);
  connect(display_widget_, &ViewerDisplayWidget::ColorManagerChanged, this, &ViewerWidget::ColorManagerChanged);
  connect(display_widget_, &ViewerDisplayWidget::DragEntered, this, &ViewerWidget::DragEntered);
  connect(display_widget_, &ViewerDisplayWidget::Dropped, this, &ViewerWidget::Dropped);
  connect(display_widget_, &ViewerDisplayWidget::TextureChanged, this, &ViewerWidget::TextureChanged);
  connect(display_widget_, &ViewerDisplayWidget::QueueStarved, this, &ViewerWidget::ForceRequeueFromCurrentTime);
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

  // FIXME: Magic number
  SetScale(48.0);

  // Ensures that seeking on the waveform view updates the time as expected
  connect(waveform_view_, &AudioWaveformView::TimeChanged, this, &ViewerWidget::SetTimeAndSignal);
  connect(waveform_view_, &AudioWaveformView::customContextMenuRequested, this, &ViewerWidget::ShowContextMenu);

  connect(&playback_backup_timer_, &QTimer::timeout, this, &ViewerWidget::PlaybackTimerUpdate);

  SetAutoMaxScrollBar(true);

  instances_.append(this);

  setAcceptDrops(true);

  connect(Core::instance(), &Core::ColorPickerEnabled, this, &ViewerWidget::SetSignalCursorColorEnabled);
  connect(this, &ViewerWidget::CursorColor, Core::instance(), &Core::ColorPickerColorEmitted);
}

ViewerWidget::~ViewerWidget()
{
  instances_.removeOne(this);

  auto windows = windows_;

  foreach (ViewerWindow* window, windows) {
    delete window;
  }
}

void ViewerWidget::TimeChangedEvent(const rational &time)
{
  if (!time_changed_from_timer_) {
    PauseInternal();
  }

  controls_->SetTime(time);
  waveform_view_->SetTime(time);

  if (GetConnectedNode() && last_time_ != time) {
    if (!IsPlaying()) {
      UpdateTextureFromNode();

      PushScrubbedAudio();

      // We don't clear the FPS timer on pause in case users want to see it immediately after, but by
      // the time a new texture is drawn, assume that the FPS no longer needs to be shown.
      display_widget_->ResetFPSTimer();
    }

    display_widget_->SetTime(time);
  }

  // Send time to auto-cacher
  auto_cacher_.SetPlayhead(time);

  last_time_ = time;
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
  connect(n, &ViewerOutput::TextureInputChanged, this, &ViewerWidget::UpdateStack);

  VideoParams vp = n->GetVideoParams();

  InterlacingChangedSlot(vp.interlacing());

  ruler()->SetPlaybackCache(n->video_frame_cache());

  SetViewerResolution(vp.width(), vp.height());
  SetViewerPixelAspect(vp.pixel_aspect_ratio());
  last_length_ = 0;
  LengthChangedSlot(n->GetLength());

  AudioParams ap = n->GetAudioParams();
  packed_processor_.Open(ap);

  ColorManager* color_manager = n->project()->color_manager();

  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->ConnectColorManager(color_manager);
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
  disconnect(n, &ViewerOutput::TextureInputChanged, this, &ViewerWidget::UpdateStack);

  packed_processor_.Close();

  SetDisplayImage(QVariant());

  ruler()->SetPlaybackCache(nullptr);

  // Effectively disables the viewer and clears the state
  SetViewerResolution(0, 0);

  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->DisconnectColorManager();
  }

  waveform_view_->SetViewer(nullptr);
  waveform_view_->ConnectTimelinePoints(nullptr);

  // Queue an UpdateStack so that when it runs, the viewer node will be fully disconnected
  QMetaObject::invokeMethod(this, &ViewerWidget::UpdateStack, Qt::QueuedConnection);
}

void ViewerWidget::ConnectedNodeChangeEvent(ViewerOutput *n)
{
  auto_cacher_.SetViewerNode(n);
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
  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->SetMatrixCrop(mat);
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

  playback_devices_.append(vw->display_widget());

  (*vw->display_widget()->queue()) = *playback_devices_.first()->queue();
  if (IsPlaying()) {
    vw->display_widget()->Play(GetTimestamp(), playback_speed_, timebase());
  }

  windows_.insert(screen, vw);
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
      && !GetConnectedNode()->GetConnectedTextureOutput()
      && GetConnectedNode()->GetConnectedSampleOutput();
}

void ViewerWidget::SetEmptyImage()
{
  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->SetBlank();
  }
}

void ViewerWidget::UpdateAutoCacher()
{
  auto_cacher_.SetPlayhead(GetTime());
}

void ViewerWidget::ClearVideoAutoCacherQueue()
{
  auto_cacher_.CancelVideoTasks();
}

void ViewerWidget::DecrementPrequeuedAudio()
{
  prequeuing_audio_--;
  if (!prequeuing_audio_) {
    FinishPlayPreprocess();
  }
}

void ViewerWidget::QueueNextAudioBuffer()
{
  rational queue_end = audio_playback_queue_time_ + (kAudioPlaybackInterval * playback_speed_);

  // Clamp queue end by zero and the audio length
  queue_end  = clamp(queue_end, rational(0), GetConnectedNode()->GetAudioLength());
  if (queue_end <= audio_playback_queue_time_) {
    // This will queue nothing, so stop the loop here
    if (prequeuing_audio_) {
      DecrementPrequeuedAudio();
    }
    return;
  }

  RenderTicketWatcher *watcher = new RenderTicketWatcher(this);
  connect(watcher, &RenderTicketWatcher::Finished, this, &ViewerWidget::ReceivedAudioBufferForPlayback);
  audio_playback_queue_.push_back(watcher);
  watcher->SetTicket(auto_cacher_.GetRangeOfAudio(TimeRange(audio_playback_queue_time_, queue_end), true));

  audio_playback_queue_time_ = queue_end;
}

void ViewerWidget::ReceivedAudioBufferForPlayback()
{
  while (!audio_playback_queue_.empty() && audio_playback_queue_.front()->HasResult()) {
    RenderTicketWatcher *watcher = audio_playback_queue_.front();
    audio_playback_queue_.pop_front();

    if (watcher->HasResult()) {
      SampleBufferPtr samples = watcher->Get().value<SampleBufferPtr>();
      if (samples) {
        // If the samples must be reversed, reverse them now
        if (playback_speed_ < 0) {
          samples->reverse();
        }

        // Convert to packed data for audio output
        QByteArray pack = packed_processor_.Convert(samples);

        // If the tempo must be adjusted, adjust now
        if (tempo_processor_.IsOpen()) {
          tempo_processor_.Push(pack.data(), pack.size());
          int actual = tempo_processor_.Pull(pack.data(), pack.size());
          if (actual != pack.size()) {
            pack.resize(actual);
          }
        }

        // TempoProcessor may have emptied the array
        if (!pack.isEmpty()) {
          if (prequeuing_audio_) {
            // Add to prequeued audio buffer
            prequeued_audio_.append(pack);
          } else {
            // Push directly to audio manager
            AudioManager::instance()->PushToOutput(GetConnectedNode()->GetAudioParams(), pack);
          }
        }
      }
    }

    if (prequeuing_audio_) {
      DecrementPrequeuedAudio();
    }

    delete watcher;
  }
}

void ViewerWidget::ReceivedAudioBufferForScrubbing()
{
  // NOTE: Might be good to organize a queue for this in the event that audio takes a long time to
  //       keep the scrubbed chunks ordered, similar to the playback_queue_ or audio_playback_queue_

  RenderTicketWatcher *watcher = static_cast<RenderTicketWatcher *>(sender());

  if (watcher->HasResult()) {
    if (SampleBufferPtr samples = watcher->Get().value<SampleBufferPtr>()) {
      if (samples->audio_params().channel_count() > 0) {
        /* Fade code
        const int kFadeSz = qMin(200, samples->sample_count()/4);
        for (int i=0; i<kFadeSz; i++) {
          float amt = float(i)/float(kFadeSz);
          samples->transform_volume_for_sample(i, amt);
          samples->transform_volume_for_sample(samples->sample_count() - i - 1, amt);
        }*/

        QByteArray packed = packed_processor_.Convert(samples);
        AudioManager::instance()->ClearBufferedOutput();
        AudioManager::instance()->PushToOutput(samples->audio_params(), packed);
        AudioMonitor::PushBytesOnAll(packed);
      }
    }
  }

  delete watcher;
}

void ViewerWidget::ForceRequeueFromCurrentTime()
{
  ClearVideoAutoCacherQueue();
  int queue = DeterminePlaybackQueueSize();
  playback_queue_next_frame_ = GetTimestamp() + playback_speed_;
  for (int i=queue_watchers_.size(); i<queue; i++) {
    RequestNextFrameForQueue();
  }
}

void ViewerWidget::UpdateTextureFromNode()
{
  if (!GetConnectedNode()) {
    return;
  }

  if (IsPlaying()) {
    qWarning() << "UpdateTextureFromNode called while playing";
    return;
  }

  rational time = GetTime();
  bool frame_exists_at_time = FrameExistsAtTime(time);
  bool frame_might_be_still = ViewerMightBeAStill();

  if (frame_exists_at_time || frame_might_be_still) {
    // Frame was not in queue, will require rendering or decoding from cache
    // Not playing, run a task to get the frame either from the cache or the renderer
    RenderTicketWatcher* watcher = new RenderTicketWatcher();
    watcher->setProperty("start", QDateTime::currentMSecsSinceEpoch());
    watcher->setProperty("time", QVariant::fromValue(time));
    connect(watcher, &RenderTicketWatcher::Finished, this, &ViewerWidget::RendererGeneratedFrame);
    nonqueue_watchers_.append(watcher);

    // Clear queue because we want this frame more than any others
    if (!GetConnectedNode()->GetVideoAutoCacheEnabled() && !auto_cacher_.IsRenderingCustomRange()) {
      ClearVideoAutoCacherQueue();
    }

    watcher->SetTicket(GetFrame(time, true));
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
      viewer->ClearVideoAutoCacherQueue();
    }

    viewer->auto_cacher_.SetAudioPaused(true);
  }

  // If the playhead is beyond the end, restart at 0
  rational last_frame = GetConnectedNode()->GetLength() - timebase();
  if (!in_to_out_only && GetTime() >= last_frame) {
    if (speed > 0) {
      SetTimeAndSignal(0);
    } else {
      SetTimeAndSignal(last_frame);
    }
  }

  playback_speed_ = speed;
  play_in_to_out_only_ = in_to_out_only;

  playback_queue_next_frame_ = GetTimestamp();

  controls_->ShowPauseButton();

  // Attempt to fill playback queue
  if (display_widget_->isVisible() || !windows_.isEmpty()) {
    prequeue_length_ = DeterminePlaybackQueueSize();

    if (prequeue_length_ > 0) {
      prequeuing_video_ = true;
      prequeue_count_ = 0;

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

  AudioParams ap = GetConnectedNode()->GetAudioParams();
  if (ap.is_valid()) {
    AudioManager::instance()->SetOutputNotifyInterval(ap.time_to_bytes(kAudioPlaybackInterval));
    connect(AudioManager::instance(), &AudioManager::OutputNotify, this, &ViewerWidget::QueueNextAudioBuffer);

    if (std::abs(playback_speed_) > 1) {
      tempo_processor_.Open(GetConnectedNode()->GetAudioParams(), std::abs(playback_speed_));
    }

    static const int prequeue_count = 2;
    prequeuing_audio_ = prequeue_count; // Queue two buffers ahead of time
    audio_playback_queue_time_ = GetTime();
    for (int i=0; i<prequeue_count; i++) {
      QueueNextAudioBuffer();
    }
  }
  // Force screen to stay awake
  PreventSleep(true);
}

void ViewerWidget::PauseInternal()
{
  if (IsPlaying()) {
    playback_speed_ = 0;
    controls_->ShowPlayButton();

    foreach (ViewerDisplayWidget *dw, playback_devices_){
      dw->Pause();
    }

    qDeleteAll(queue_watchers_);
    queue_watchers_.clear();

    playback_backup_timer_.stop();

    // Handle audio
    AudioManager::instance()->StopOutput();
    AudioMonitor::StopOnAll();
    prequeued_audio_.clear();
    disconnect(AudioManager::instance(), &AudioManager::OutputNotify, this, &ViewerWidget::QueueNextAudioBuffer);
    qDeleteAll(audio_playback_queue_);
    audio_playback_queue_.clear();
    if (tempo_processor_.IsOpen()) {
      tempo_processor_.Close();
    }

    foreach (ViewerWidget* viewer, instances_) {
      viewer->auto_cacher_.SetAudioPaused(false);
    }

    UpdateTextureFromNode();
  }

  prequeuing_video_ = false;
  prequeuing_audio_ = 0;

  // Reset screen timeout timer
  PreventSleep(false);
}

void ViewerWidget::PushScrubbedAudio()
{
  if (!IsPlaying() && GetConnectedNode() && Config::Current()[QStringLiteral("AudioScrubbing")].toBool()) {
    // Get audio src device from renderer
    const AudioParams& params = GetConnectedNode()->audio_playback_cache()->GetParameters();

    if (params.is_valid()) {
      // NOTE: Hardcoded scrubbing interval (20ms)
      rational interval = rational(20, 1000);

      RenderTicketWatcher *watcher = new RenderTicketWatcher();
      connect(watcher, &RenderTicketWatcher::Finished, this, &ViewerWidget::ReceivedAudioBufferForScrubbing);
      watcher->SetTicket(auto_cacher_.GetRangeOfAudio(TimeRange(GetTime(), GetTime() + interval), true));
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
  return GetConnectedNode() && GetConnectedNode()->GetConnectedTextureOutput() && GetConnectedNode()->GetVideoLength().isNull();
}

void ViewerWidget::SetDisplayImage(QVariant frame)
{
  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->SetImage(frame);
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
    queue_watchers_.append(watcher);
    watcher->SetTicket(GetFrame(next_time, prioritize));
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
  // Check if we're still waiting for video or audio respectively
  if (prequeuing_video_ || prequeuing_audio_) {
    return;
  }

  int64_t playback_start_time = GetTimestamp();

  // Start audio waveform playback
  if (!prequeued_audio_.isEmpty()) {
    AudioManager::instance()->PushToOutput(GetConnectedNode()->GetAudioParams(), prequeued_audio_);
    prequeued_audio_.clear();

    AudioMonitor::StartWaveformOnAll(&GetConnectedNode()->audio_playback_cache()->visual(),
                                     GetTime(), playback_speed_);
  }

  display_widget_->ResetFPSTimer();

  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->Play(playback_start_time, playback_speed_, timebase());
  }

  // This is our timer for loading the queue and setting the time
  playback_backup_timer_.setInterval(qFloor(timebase_dbl()));
  playback_backup_timer_.start();

  PlaybackTimerUpdate();
}

int ViewerWidget::DeterminePlaybackQueueSize()
{
  if (playback_speed_ == 0) {
    return 0;
  }

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
  ViewerWindow *vw = static_cast<ViewerWindow*>(sender());
  windows_.remove(windows_.key(vw));
  playback_devices_.removeOne(vw->display_widget());
}

void ViewerWidget::RendererGeneratedFrame()
{
  RenderTicketWatcher* ticket = static_cast<RenderTicketWatcher*>(sender());

  if (ticket->HasResult()) {
    if (nonqueue_watchers_.contains(ticket)) {
      while (!nonqueue_watchers_.isEmpty()) {
        // Pop frames that are "old"
        if (nonqueue_watchers_.takeFirst() == ticket) {
          break;
        }
      }

      // If the frame we received is not the most recent frame we're waiting for (i.e. if nonqueue_watchers_
      // is not empty), we discard the frame - because otherwise if frames are taking a while (i.e.
      // uncached frames) it can be a little disconcerting to the user for a bunch of frames to
      // slowly go by - UNLESS the time it took to make this frame was fairly short (under 100ms here),
      // in which case, we DO show it because it can be disconcerting in the other direction to see
      // a scrub just jump to the final frame without seeing the frames in between. It's just a
      // little nicer to get to see that in that situation, so we handle both.
      qint64 frame_start_time = ticket->property("start").value<qint64>();
      if (nonqueue_watchers_.isEmpty() || (QDateTime::currentMSecsSinceEpoch() - frame_start_time < 100)) {
        SetDisplayImage(ticket->Get());
      }
    }
  }

  delete ticket;
}

void ViewerWidget::RendererGeneratedFrameForQueue()
{
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  if (queue_watchers_.contains(watcher)) {
    queue_watchers_.removeOne(watcher);

    if (watcher->HasResult()) {
      QVariant frame = watcher->Get();

      // Ignore this signal if we've paused now
      if (IsPlaying() || prequeuing_video_) {
        rational ts = watcher->property("time").value<rational>();

        foreach (ViewerDisplayWidget *dw, playback_devices_) {
          dw->queue()->AppendTimewise({ts, frame}, playback_speed_);
        }
        prequeue_count_++;

        if (prequeuing_video_ && prequeue_count_ == prequeue_length_) {
          prequeuing_video_ = false;
          FinishPlayPreprocess();
        }
      }
    }
  }

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
      Menu* cache_menu = new Menu(tr("Cache"), &menu);
      menu.addMenu(cache_menu);

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
    QAction *stop_playback_on_last_frame = menu.addAction(tr("Stop Playback On Last Frame"));
    stop_playback_on_last_frame->setCheckable(true);
    stop_playback_on_last_frame->setChecked(Config::Current()[QStringLiteral("StopPlaybackOnLastFrame")].toBool());
    connect(stop_playback_on_last_frame, &QAction::triggered, this, [](bool e){
      Config::Current()[QStringLiteral("StopPlaybackOnLastFrame")] = e;
    });

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
      SetTimeAndSignal(GetConnectedNode()->GetTimelinePoints()->workarea()->in());
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
  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->SetSignalCursorColorEnabled(e);
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
  Q_ASSERT(playback_speed_ != 0);

  rational current_time = Timecode::timestamp_to_time(display_widget_->timer()->GetTimestampNow(), timebase());

  rational min_time, max_time;

  if (play_in_to_out_only_ && GetConnectedNode()->GetTimelinePoints()->workarea()->enabled()) {

    // If "play in to out" is enabled or we're looping AND we have a workarea, only play the workarea
    min_time = GetConnectedNode()->GetTimelinePoints()->workarea()->in();
    max_time = GetConnectedNode()->GetTimelinePoints()->workarea()->out();

  } else {

    // Otherwise set the bounds to the range of the sequence
    min_time = 0;
    max_time = GetConnectedNode()->GetLength();

  }

  // If we're stopping playback on the last frame rather than after it, subtract our max time
  // by one timebase unit
  if (Config::Current()[QStringLiteral("StopPlaybackOnLastFrame")].toBool()) {
    max_time = qMax(min_time, max_time - timebase());
  }

  rational time_to_set;
  bool end_of_line = false;
  bool play_after_pause = false;

  if ((playback_speed_ < 0 && current_time <= min_time)
      || (playback_speed_ > 0 && current_time >= max_time)) {

    // Determine which timestamp we tripped
    rational tripped_time;

    if (current_time <= min_time) {
      tripped_time = min_time;
    } else {
      tripped_time = max_time;
    }

    // Signal that we've reached the end of whatever range we're playing and should either pause
    // or restart playback
    end_of_line = true;

    if (Config::Current()[QStringLiteral("Loop")].toBool()) {

      // If we're looping, jump to the other side of the workarea and continue
      time_to_set = (tripped_time == min_time) ? max_time : min_time;

      // Signal to restart playback after the pause signalled by `end_of_line`
      play_after_pause = true;

    } else {

      // Pause at the boundary we tripped
      time_to_set = tripped_time;

    }

  } else {

    // Sets time normally to whatever we calculated as the "current time"
    time_to_set = current_time;

  }

  // Set the time. By wrapping in this bool, we prevent TimeChangedEvent's default behavior of
  // pausing. Even if we pause it later with `end_of_line`, we prefer pausing after setting the time
  // so that an audio scrub event, etc. isn't sent.
  time_changed_from_timer_ = true;
  SetTimeAndSignal(time_to_set);
  time_changed_from_timer_ = false;
  if (end_of_line) {
    // Cache the current speed
    int current_speed = playback_speed_;

    PauseInternal();
    if (play_after_pause) {
      PlayInternal(current_speed, play_in_to_out_only_);
    }
  }

  if (IsPlaying()) {
    int count = 0;
    for (int i=display_widget_->queue()->size(); i<DeterminePlaybackQueueSize(); i++) {
      RequestNextFrameForQueue();
      count++;
    }
  }

  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->queue()->PurgeBefore(current_time, playback_speed_);
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
    controls_->SetEndTime(length);
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
  bool deint = interlacing != VideoParams::kInterlaceNone;

  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->SetDeinterlacing(deint);
  }
}

void ViewerWidget::UpdateRendererVideoParameters()
{
  VideoParams vp = GetConnectedNode()->GetVideoParams();

  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->SetVideoParams(vp);
  }
}

void ViewerWidget::UpdateRendererAudioParameters()
{
  packed_processor_.Close();

  AudioParams ap = GetConnectedNode()->GetAudioParams();

  packed_processor_.Open(ap);
}

void ViewerWidget::SetZoomFromMenu(QAction *action)
{
  sizer_->SetZoom(action->data().toInt());
}

void ViewerWidget::ViewerInvalidatedVideoRange(const TimeRange &range)
{
  // If our current frame is within this range, we need to update
  if (GetTime() >= range.in() && (GetTime() < range.out() || range.in() == range.out())) {
    QMetaObject::invokeMethod(this, &ViewerWidget::UpdateTextureFromNode, Qt::QueuedConnection);
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

void ViewerWidget::ViewerShiftedRange(const rational &from, const rational &to)
{
  if (GetTime() >= qMin(from, to)) {
    QMetaObject::invokeMethod(this, &ViewerWidget::UpdateTextureFromNode, Qt::QueuedConnection);
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

}
