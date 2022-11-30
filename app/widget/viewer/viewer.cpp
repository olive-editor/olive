/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include <QFontDialog>
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
#include "common/ratiodialog.h"
#include "common/timecodefunctions.h"
#include "config/config.h"
#include "core.h"
#include "node/block/gap/gap.h"
#include "node/generator/shape/shapenodebase.h"
#include "node/project/project.h"
#include "panel/multicam/multicampanel.h"
#include "panel/panelmanager.h"
#include "render/rendermanager.h"
#include "viewerpreventsleep.h"
#include "widget/audiomonitor/audiomonitor.h"
#include "widget/menu/menu.h"
#include "widget/multicam/multicamdisplay.h"
#include "widget/nodeparamview/nodeparamviewundo.h"
#include "widget/timelinewidget/tool/add.h"
#include "widget/timeruler/timeruler.h"

namespace olive {

#define super TimeBasedWidget

QVector<ViewerWidget*> ViewerWidget::instances_;

// NOTE: Hardcoded interval of size of audio chunk to render and send to the output at a time.
//       We want this to be as long as possible so the code has plenty of time to send the audio
//       while also being as short as possible so users get relatively immediate feedback when
//       changing values. 1/4 second seems to be a good middleground.
const rational ViewerWidget::kAudioPlaybackInterval = rational(1, 4);

const rational kVideoPlaybackInterval = rational(1, 2);

ViewerWidget::ViewerWidget(ViewerDisplayWidget *display, QWidget *parent) :
  super(false, true, parent),
  playback_speed_(0),
  color_menu_enabled_(true),
  time_changed_from_timer_(false),
  prequeuing_video_(false),
  prequeuing_audio_(0),
  record_armed_(false),
  recording_(false),
  first_requeue_watcher_(nullptr),
  enable_audio_scrubbing_(true),
  waveform_mode_(kWFAutomatic),
  ignore_scrub_(0),
  multicam_panel_(nullptr)
{
  // Set up main layout
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  // Create main OpenGL-based view and sizer
  sizer_ = new ViewerSizer();
  layout->addWidget(sizer_);

  display_widget_ = display;
  display_widget_->SetShowWidgetBackground(true);
  playback_devices_.append(display_widget_);
  connect(display_widget_, &ViewerDisplayWidget::customContextMenuRequested, this, &ViewerWidget::ShowContextMenu);
  connect(display_widget_, &ViewerDisplayWidget::CursorColor, this, &ViewerWidget::CursorColor);
  connect(display_widget_, &ViewerDisplayWidget::ColorProcessorChanged, this, &ViewerWidget::ColorProcessorChanged);
  connect(display_widget_, &ViewerDisplayWidget::ColorManagerChanged, this, &ViewerWidget::ColorManagerChanged);
  connect(display_widget_, &ViewerDisplayWidget::DragEntered, this, &ViewerWidget::DragEntered);
  connect(display_widget_, &ViewerDisplayWidget::Dropped, this, &ViewerWidget::Dropped);
  connect(display_widget_, &ViewerDisplayWidget::TextureChanged, this, &ViewerWidget::TextureChanged);
  connect(display_widget_, &ViewerDisplayWidget::QueueStarved, this, &ViewerWidget::QueueStarved);
  connect(display_widget_, &ViewerDisplayWidget::QueueNoLongerStarved, this, &ViewerWidget::QueueNoLongerStarved);
  connect(display_widget_, &ViewerDisplayWidget::CreateAddableAt, this, &ViewerWidget::CreateAddableAt);
  connect(sizer_, &ViewerSizer::RequestScale, display_widget_, &ViewerDisplayWidget::SetMatrixZoom);
  connect(sizer_, &ViewerSizer::RequestTranslate, display_widget_, &ViewerDisplayWidget::SetMatrixTranslate);
  connect(display_widget_, &ViewerDisplayWidget::HandDragMoved, sizer_, &ViewerSizer::HandDragMove);
  sizer_->SetWidget(display_widget_);

  // Make the display widget the first tabbable widget. While the viewer display cannot actually
  // be interacted with by tabbing, it prevents the actual first tabbable widget (the playhead
  // slider in `controls_`) from getting auto-focused any time the panel is maximized (with `)
  display_widget_->setFocusPolicy(Qt::TabFocus);

  // Create waveform view when audio is connected and video isn't
  waveform_view_ = new AudioWaveformView();
  ConnectTimelineView(waveform_view_);
  layout->addWidget(waveform_view_);

  // Create time ruler
  layout->addWidget(ruler());

  // Create scrollbar
  layout->addWidget(scrollbar());

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
  layout->addWidget(controls_);

  // FIXME: Magic number
  SetScale(48.0);

  // Ensures that seeking on the waveform view updates the time as expected
  connect(waveform_view_, &AudioWaveformView::customContextMenuRequested, this, &ViewerWidget::ShowContextMenu);

  connect(&playback_backup_timer_, &QTimer::timeout, this, &ViewerWidget::PlaybackTimerUpdate);

  SetAutoMaxScrollBar(true);

  instances_.append(this);

  auto_cacher_ = new PreviewAutoCacher(this);
  connect(display_widget_, &ViewerDisplayWidget::ColorProcessorChanged, auto_cacher_, &PreviewAutoCacher::SetDisplayColorProcessor);

  UpdateWaveformViewFromMode();

  connect(Core::instance(), &Core::ColorPickerEnabled, this, &ViewerWidget::SetSignalCursorColorEnabled);
  connect(this, &ViewerWidget::CursorColor, Core::instance(), &Core::ColorPickerColorEmitted);
  connect(AudioManager::instance(), &AudioManager::OutputParamsChanged, this, &ViewerWidget::UpdateAudioProcessor);
}

ViewerWidget::~ViewerWidget()
{
  instances_.removeOne(this);

  auto windows = windows_;

  foreach (ViewerWindow* window, windows) {
    delete window;
  }

  delete display_widget_;
  display_widget_ = nullptr;
}

void ViewerWidget::TimeChangedEvent(const rational &time)
{
  if (!time_changed_from_timer_) {
    PauseInternal();
  }

  if (record_armed_) {
    DisarmRecording();
  }

  controls_->SetTime(time);

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
  auto_cacher_->SetPlayhead(time);

  last_time_ = time;
}

void ViewerWidget::ConnectNodeEvent(ViewerOutput *n)
{
  connect(n, &ViewerOutput::SizeChanged, this, &ViewerWidget::SetViewerResolution);
  connect(n, &ViewerOutput::PixelAspectChanged, this, &ViewerWidget::SetViewerPixelAspect);
  connect(n, &ViewerOutput::LengthChanged, this, &ViewerWidget::LengthChangedSlot);
  connect(n, &ViewerOutput::InterlacingChanged, this, &ViewerWidget::InterlacingChangedSlot);
  connect(n, &ViewerOutput::VideoParamsChanged, this, &ViewerWidget::UpdateRendererVideoParameters);
  connect(n, &ViewerOutput::VideoParamsChanged, this, &ViewerWidget::UpdateTextureFromNode, Qt::QueuedConnection);
  connect(n, &ViewerOutput::AudioParamsChanged, this, &ViewerWidget::UpdateRendererAudioParameters);
  connect(n->video_frame_cache(), &FrameHashCache::Invalidated, this, &ViewerWidget::ViewerInvalidatedVideoRange);
  connect(n, &ViewerOutput::TextureInputChanged, this, &ViewerWidget::UpdateWaveformViewFromMode);

  connect(controls_, &PlaybackControls::TimeChanged, n, &ViewerOutput::SetPlayhead);

  VideoParams vp = n->GetVideoParams();

  InterlacingChangedSlot(vp.interlacing());

  ruler()->SetPlaybackCache(n->video_frame_cache());

  SetViewerResolution(vp.width(), vp.height());
  SetViewerPixelAspect(vp.pixel_aspect_ratio());
  last_length_ = 0;
  LengthChangedSlot(n->GetLength());

  UpdateAudioProcessor();

  ColorManager* color_manager = n->project()->color_manager();

  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->ConnectColorManager(color_manager);
  }

  UpdateWaveformViewFromMode();

  waveform_view_->SetViewer(GetConnectedNode());

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
  disconnect(n, &ViewerOutput::VideoParamsChanged, this, &ViewerWidget::UpdateTextureFromNode);
  disconnect(n, &ViewerOutput::AudioParamsChanged, this, &ViewerWidget::UpdateRendererAudioParameters);
  disconnect(n->video_frame_cache(), &FrameHashCache::Invalidated, this, &ViewerWidget::ViewerInvalidatedVideoRange);
  disconnect(n, &ViewerOutput::TextureInputChanged, this, &ViewerWidget::UpdateWaveformViewFromMode);

  disconnect(controls_, &PlaybackControls::TimeChanged, n, &ViewerOutput::SetPlayhead);

  timeline_selected_blocks_.clear();
  node_view_selected_.clear();
  if (multicam_panel_) {
    multicam_panel_->SetMulticamNode(nullptr, nullptr, nullptr, rational::NaN);
  }

  CloseAudioProcessor();
  audio_scrub_watchers_.clear();

  SetDisplayImage(nullptr);

  ruler()->SetPlaybackCache(nullptr);

  // Effectively disables the viewer and clears the state
  SetViewerResolution(0, 0);

  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->DisconnectColorManager();
  }

  waveform_view_->SetViewer(nullptr);

  // Queue an UpdateStack so that when it runs, the viewer node will be fully disconnected
  QMetaObject::invokeMethod(this, &ViewerWidget::UpdateWaveformViewFromMode, Qt::QueuedConnection);

  SetGizmos(nullptr);
}

void ViewerWidget::ConnectedNodeChangeEvent(ViewerOutput *n)
{
  auto_cacher_->SetViewerNode(n);
  display_widget_->SetSubtitleTracks(dynamic_cast<Sequence*>(n));
}

void ViewerWidget::ConnectedWorkAreaChangeEvent(TimelineWorkArea *workarea)
{
  waveform_view_->SetWorkArea(workarea);
}

void ViewerWidget::ConnectedMarkersChangeEvent(TimelineMarkerList *markers)
{
  waveform_view_->SetMarkers(markers);
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
    vw->display_widget()->Play(GetTimestamp(), playback_speed_, timebase(), true);
  }

  windows_.insert(screen, vw);
}

void ViewerWidget::CacheEntireSequence()
{
  auto_cacher_->ForceCacheRange(TimeRange(0, GetConnectedNode()->GetVideoLength()));
}

void ViewerWidget::CacheSequenceInOut()
{
  if (GetConnectedNode() && GetConnectedNode()->GetWorkArea()->enabled()) {
    auto_cacher_->ForceCacheRange(GetConnectedNode()->GetWorkArea()->range());
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

void ViewerWidget::StartCapture(TimelineWidget *source, const TimeRange &time, const Track::Reference &track)
{
  GetConnectedNode()->SetPlayhead(time.in());
  ArmForRecording();

  recording_callback_ = source;
  recording_range_ = time;
  recording_track_ = track;
}

void ViewerWidget::ConnectMulticamWidget(MulticamWidget *p)
{
  if (multicam_panel_) {
    disconnect(multicam_panel_, &MulticamWidget::Switched, this, &ViewerWidget::DetectMulticamNodeNow);
  }

  multicam_panel_ = p;

  if (multicam_panel_) {
    connect(multicam_panel_, &MulticamWidget::Switched, this, &ViewerWidget::DetectMulticamNodeNow);
  }
}

FramePtr ViewerWidget::DecodeCachedImage(const QString &cache_path, const QUuid &cache_id, const int64_t& time)
{
  FramePtr frame = FrameHashCache::LoadCacheFrame(cache_path, cache_id, time);

  if (frame) {
    frame->set_timestamp(time);
  } else {
    qWarning() << "Tried to load cached frame from file but it was null";
  }

  return frame;
}

void ViewerWidget::DecodeCachedImage(RenderTicketPtr ticket, const QString &cache_path, const QUuid &cache_id, const int64_t& time)
{
  ticket->Start();

  FramePtr f = DecodeCachedImage(cache_path, cache_id, time);

  if (f) {
    ticket->Finish(QVariant::fromValue(f));
  } else {
    ticket->Finish();
  }
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
  auto_cacher_->SetPlayhead(GetConnectedNode()->GetPlayhead());
}

void ViewerWidget::DecrementPrequeuedAudio()
{
  prequeuing_audio_--;
  if (!prequeuing_audio_) {
    FinishPlayPreprocess();
  }
}

void ViewerWidget::ArmForRecording()
{
  controls_->StartPlayBlink();
  record_armed_ = true;
}

void ViewerWidget::DisarmRecording()
{
  controls_->StopPlayBlink();
  record_armed_ = false;
}

void ViewerWidget::UpdateAudioProcessor()
{
  if (GetConnectedNode()) {
    CloseAudioProcessor();

    AudioParams ap = GetConnectedNode()->GetAudioParams();
    AudioParams packed(OLIVE_CONFIG("AudioOutputSampleRate").toInt(),
                       OLIVE_CONFIG("AudioOutputChannelLayout").toULongLong(),
                       static_cast<AudioParams::Format>(OLIVE_CONFIG("AudioOutputSampleFormat").toInt()));

    audio_processor_.Open(ap, packed, (playback_speed_ == 0) ? 1 : std::abs(playback_speed_));
  }
}

void ViewerWidget::CreateAddableAt(const QRectF &f)
{
  if (Sequence *s = dynamic_cast<Sequence*>(GetConnectedNode())) {
    Track::Type type = Track::kVideo;
    int track_index = -1;
    TrackList *list = s->track_list(type);
    const rational &in = GetConnectedNode()->GetPlayhead();
    rational length = OLIVE_CONFIG("DefaultStillLength").value<rational>();
    rational out = in + length;

    // Find a free track where we won't overwrite anything
    while (true) {
      track_index++;

      if (track_index >= list->GetTrackCount()) {
        // Just create a new track
        break;
      }

      Track *track = list->GetTrackAt(track_index);
      if (track->IsLocked()) {
        continue;
      }

      Block *b = track->NearestBlockBeforeOrAt(in);
      if (!b || (dynamic_cast<GapBlock*>(b) && b->out() >= out)) {
        break;
      }
    }

    MultiUndoCommand *command = new MultiUndoCommand();
    Node *clip = AddTool::CreateAddableClip(command, s, Track::Reference(type, track_index), in, length);

    if (ShapeNodeBase *shape = dynamic_cast<ShapeNodeBase*>(clip)) {
      shape->SetRect(f, s->GetVideoParams(), command);
    }

    Core::instance()->undo_stack()->pushIfHasChildren(command);
    SetGizmos(clip);
  }
}

void ViewerWidget::HandleFirstRequeueDestroy()
{
  // Extra protection to ensure we don't reference a destroyed object
  if (first_requeue_watcher_ == sender()) {
    first_requeue_watcher_ = nullptr;
  }
}

void ViewerWidget::ShowSubtitleProperties()
{
  QFont f(OLIVE_CONFIG("DefaultSubtitleFamily").toString(), OLIVE_CONFIG("DefaultSubtitleSize").toInt(), OLIVE_CONFIG("DefaultSubtitleWeight").toInt());
  QFontDialog fd(f, this);

  if (fd.exec() == QDialog::Accepted) {
    f = fd.selectedFont();
    OLIVE_CONFIG("DefaultSubtitleSize") = f.pointSize();
    OLIVE_CONFIG("DefaultSubtitleFamily") = f.family();
    OLIVE_CONFIG("DefaultSubtitleWeight") = f.weight();
    display_widget_->update();
  }
}

void ViewerWidget::DryRunFinished()
{
  RenderTicketWatcher *w = static_cast<RenderTicketWatcher*>(sender());

  if (dry_run_watchers_.contains(w)) {
    RequestNextDryRun();
  }

  delete w;
}

void ViewerWidget::RequestNextDryRun()
{
  if (IsPlaying()) {
    rational next_time = Timecode::timestamp_to_time(dry_run_next_frame_, timebase());
    if (FrameExistsAtTime(next_time)) {
      if (next_time > GetConnectedNode()->GetPlayhead() + RenderManager::kDryRunInterval) {
        QTimer::singleShot(timebase().toDouble() / playback_speed_, this, &ViewerWidget::RequestNextDryRun);
      } else {
        RenderTicketWatcher *watcher = new RenderTicketWatcher(this);
        connect(watcher, &RenderTicketWatcher::Finished, this, &ViewerWidget::DryRunFinished);
        watcher->SetTicket(GetSingleFrame(next_time, true));
        dry_run_next_frame_ += playback_speed_;
        dry_run_watchers_.append(watcher);
      }
    }
  }
}

void ViewerWidget::SaveFrameAsImage()
{
  Core::instance()->OpenExportDialogForViewer(GetConnectedNode(), true);
}

void ViewerWidget::DetectMulticamNodeNow()
{
  if (GetConnectedNode()) {
    DetectMulticamNode(GetConnectedNode()->GetPlayhead());
  }
}

void ViewerWidget::CloseAudioProcessor()
{
  audio_processor_.Close();
}

void ViewerWidget::SetWaveformMode(WaveformMode wf)
{
  waveform_mode_ = wf;
  UpdateWaveformViewFromMode();
}

void ViewerWidget::DetectMulticamNode(const rational &time)
{
  // Look for multicam node
  MultiCamNode *multicam = nullptr;
  ClipBlock *clip = nullptr;

  // Faster way to do this
  if (multicam_panel_ && multicam_panel_->isVisible()) {
    if (Sequence *s = dynamic_cast<Sequence*>(GetConnectedNode())) {
      // Prefer selected nodes
      for (Node *n : qAsConst(node_view_selected_)) {
        if ((multicam = dynamic_cast<MultiCamNode*>(n))) {
          // Found multicam, now try to find corresponding clip from selected timeline blocks
          for (Block *b : qAsConst(timeline_selected_blocks_)) {
            if (ClipBlock *c = dynamic_cast<ClipBlock*>(b)) {
              if (c->range().Contains(time) && c->ContextContainsNode(multicam)) {
                clip = c;
                break;
              }
            }
          }
          break;
        }
      }

      // Next, prefer multicam from selected block
      if (!multicam) {
        for (Block *b : qAsConst(timeline_selected_blocks_)) {
          if (b->range().Contains(time)) {
            if ((clip = dynamic_cast<ClipBlock*>(b))) {
              if ((multicam = clip->FindMulticam())) {
                break;
              }
            }
          }
        }
      }

      if (!multicam) {
        const QVector<Track*> &tracks = s->GetTracks();
        for (Track *t : tracks) {
          if (t->IsLocked()) {
            continue;
          }

          Block *b = t->NearestBlockBeforeOrAt(time);
          if ((clip = dynamic_cast<ClipBlock*>(b))) {
            if ((multicam = clip->FindMulticam())) {
              break;
            }
          }
        }
      }
    }
  }

  if (multicam) {
    if (multicam_panel_) {
      multicam_panel_->SetMulticamNode(GetConnectedNode(), multicam, clip, time);
    }
    auto_cacher()->SetMulticamNode(multicam);
  } else {
    auto_cacher()->SetMulticamNode(nullptr);
    if (multicam_panel_) {
      multicam_panel_->SetMulticamNode(nullptr, nullptr, nullptr, time);
    }
  }
}

bool ViewerWidget::IsVideoVisible() const
{
  return GetConnectedNode()->GetVideoParams().video_type() != VideoParams::kVideoTypeStill
      && (display_widget_->isVisible() || !windows_.isEmpty());
}

void ViewerWidget::UpdateWaveformViewFromMode()
{
  bool prefer_waveform = ShouldForceWaveform();

  sizer_->setVisible(waveform_mode_ == kWFViewerAndWaveform || waveform_mode_ == kWFViewerOnly || (waveform_mode_ == kWFAutomatic && !prefer_waveform));
  waveform_view_->setVisible(waveform_mode_ == kWFViewerAndWaveform || waveform_mode_ == kWFWaveformOnly || (waveform_mode_ == kWFAutomatic && prefer_waveform));

  waveform_view_->setSizePolicy(QSizePolicy::Expanding, waveform_mode_ == kWFViewerAndWaveform ? QSizePolicy::Maximum : QSizePolicy::Expanding);

  if (GetConnectedNode()) {
    GetConnectedNode()->SetWaveformEnabled(waveform_view_->isVisible());
  }
}

void ViewerWidget::QueueNextAudioBuffer()
{
  rational queue_end = audio_playback_queue_time_ + (kAudioPlaybackInterval * playback_speed_);

  // Clamp queue end by zero and the audio length
  queue_end  = clamp(queue_end, rational(0), GetConnectedNode()->GetAudioLength());
  if ((playback_speed_ > 0 && queue_end <= audio_playback_queue_time_)
      || (playback_speed_ < 0 && queue_end >= audio_playback_queue_time_)) {
    // This will queue nothing, so stop the loop here
    if (prequeuing_audio_) {
      DecrementPrequeuedAudio();
    }
    return;
  }

  RenderTicketWatcher *watcher = new RenderTicketWatcher(this);
  connect(watcher, &RenderTicketWatcher::Finished, this, &ViewerWidget::ReceivedAudioBufferForPlayback);
  audio_playback_queue_.push_back(watcher);
  watcher->SetTicket(auto_cacher_->GetRangeOfAudio(TimeRange(audio_playback_queue_time_, queue_end)));

  audio_playback_queue_time_ = queue_end;
}

void ViewerWidget::ReceivedAudioBufferForPlayback()
{
  while (!audio_playback_queue_.empty() && audio_playback_queue_.front()->HasResult()) {
    RenderTicketWatcher *watcher = audio_playback_queue_.front();
    audio_playback_queue_.pop_front();

    if (watcher->HasResult()) {
      SampleBuffer samples = watcher->Get().value<SampleBuffer>();
      if (samples.is_allocated()) {
        // If the samples must be reversed, reverse them now
        if (playback_speed_ < 0) {
          samples.reverse();
        }

        // Convert to packed data for audio output
        AudioProcessor::Buffer buf;
        int r = audio_processor_.Convert(samples.to_raw_ptrs().data(), samples.sample_count(), &buf);

        // TempoProcessor may have emptied the array
        if (r >= 0) {
          if (!buf.empty()) {
            const QByteArray &pack = buf.at(0);
            if (prequeuing_audio_) {
              // Add to prequeued audio buffer
              prequeued_audio_.append(pack);
            } else {
              // Push directly to audio manager
              AudioManager::instance()->PushToOutput(audio_processor_.to(), pack);
            }
          }
        } else {
          qCritical() << "Failed to process audio for playback:" << r;
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
  RenderTicketWatcher *watcher = static_cast<RenderTicketWatcher *>(sender());

  while (!audio_scrub_watchers_.empty() && audio_scrub_watchers_.front() != watcher) {
    audio_scrub_watchers_.pop_front();
  }

  if (!audio_scrub_watchers_.empty()) {
    if (watcher->HasResult()) {
      SampleBuffer samples = watcher->Get().value<SampleBuffer>();
      if (samples.is_allocated()) {
        if (samples.audio_params().channel_count() > 0) {
          AudioProcessor::Buffer buf;
          int r = audio_processor_.Convert(samples.to_raw_ptrs().data(), samples.sample_count(), &buf);

          if (r >= 0) {
            if (!buf.empty()) {
              QString error;
              const QByteArray &packed = buf.at(0);
              AudioManager::instance()->ClearBufferedOutput();
              if (!AudioManager::instance()->PushToOutput(audio_processor_.to(), packed, &error)) {
                Core::instance()->ShowStatusBarMessage(tr("Audio scrubbing failed: %1").arg(error));
              }
              AudioMonitor::PushSampleBufferOnAll(samples);
            }
          } else {
            qCritical() << "Failed to process audio for scrubbing:" << r;
          }
        }
      }
    }
  }

  delete watcher;
}

void ViewerWidget::QueueStarved()
{
  static const int kMaximumWaitTimeMs = 250;
  static const rational kMaximumWaitTime(kMaximumWaitTimeMs, 1000);
  qint64 now = QDateTime::currentMSecsSinceEpoch();

  if (!queue_starved_start_) {
    queue_starved_start_ = now;
  } else if (now > queue_starved_start_ + kMaximumWaitTimeMs) {
    if (first_requeue_watcher_) {
      if (GetConnectedNode()->GetPlayhead() + kMaximumWaitTime < first_requeue_watcher_->property("time").value<rational>()) {
        // We still have time
        return;
      }
    }

    ForceRequeueFromCurrentTime();
    queue_starved_start_ = 0;
  }
}

void ViewerWidget::QueueNoLongerStarved()
{
  queue_starved_start_ = 0;
}

void ViewerWidget::ForceRequeueFromCurrentTime()
{
  // Allow half a second for requeue to complete
  static const rational kRequeueWaitTime(1);

  auto_cacher_->ClearSingleFrameRenders();
  queue_watchers_.clear();
  int queue = DeterminePlaybackQueueSize();
  playback_queue_next_frame_ = GetTimestamp() + playback_speed_ * Timecode::time_to_timestamp(kRequeueWaitTime, timebase(), Timecode::kFloor);;
  first_requeue_watcher_ = nullptr;
  for (int i=0; i<queue; i++) {
    RenderTicketWatcher *watcher = RequestNextFrameForQueue();
    if (!first_requeue_watcher_) {
      first_requeue_watcher_ = watcher;
      connect(first_requeue_watcher_, &RenderTicketWatcher::destroyed, this, &ViewerWidget::HandleFirstRequeueDestroy);
    }
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

  rational time = GetConnectedNode()->GetPlayhead();
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
    auto_cacher_->ClearSingleFrameRenders();

    DetectMulticamNode(time);

    watcher->SetTicket(GetFrame(time));
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
    }
    viewer->auto_cacher_->SetThumbnailsPaused(true);
  }

  RenderManager::instance()->SetAggressiveGarbageCollection(true);

  // Disarm recording if armed
  if (record_armed_) {
    DisarmRecording();
  }

  // If the playhead is beyond the end, restart at 0
  if (!recording_) {
    rational last_frame = GetConnectedNode()->GetLength() - timebase();
    if (!in_to_out_only && GetConnectedNode()->GetPlayhead() >= last_frame) {
      if (speed > 0) {
        GetConnectedNode()->SetPlayhead(0);
      } else {
        GetConnectedNode()->SetPlayhead(last_frame);
      }
    }
  }

  playback_speed_ = speed;
  play_in_to_out_only_ = in_to_out_only;

  playback_queue_next_frame_ = GetTimestamp() + playback_speed_;

  controls_->ShowPauseButton();

  queue_starved_start_ = 0;

  // Attempt to fill playback queue
  if (IsVideoVisible()) {
    prequeue_length_ = DeterminePlaybackQueueSize();

    if (prequeue_length_ > 0) {
      prequeuing_video_ = true;
      prequeue_count_ = 0;

      for (int i=0; i<prequeue_length_; i++) {
        RequestNextFrameForQueue();
      }

      dry_run_next_frame_ = playback_queue_next_frame_;
      RequestNextDryRun();
    }
  }

  AudioParams ap = GetConnectedNode()->GetAudioParams();
  if (ap.is_valid()) {
    UpdateAudioProcessor();

    AudioManager::instance()->SetOutputNotifyInterval(audio_processor_.to().time_to_bytes(kAudioPlaybackInterval));
    connect(AudioManager::instance(), &AudioManager::OutputNotify, this, &ViewerWidget::QueueNextAudioBuffer);

    static const int prequeue_count = 2;
    prequeuing_audio_ = prequeue_count; // Queue two buffers ahead of time
    audio_playback_queue_time_ = GetConnectedNode()->GetPlayhead();
    for (int i=0; i<prequeue_count; i++) {
      QueueNextAudioBuffer();
    }
  }

  // Force screen to stay awake
  PreventSleep(true);
}

void ViewerWidget::PauseInternal()
{
  if (recording_) {
    AudioManager::instance()->StopRecording();
    recording_ = false;
    controls_->SetPauseButtonRecordingState(false);

    recording_callback_->DisableRecordingOverlay();
    recording_callback_->RecordingCallback(recording_filename_, recording_range_, recording_track_);
  }

  if (IsPlaying()) {
    playback_speed_ = 0;
    controls_->ShowPlayButton();

    foreach (ViewerDisplayWidget *dw, playback_devices_){
      dw->Pause();
    }

    qDeleteAll(queue_watchers_);
    queue_watchers_.clear();
    auto_cacher_->ClearSingleFrameRenders();

    playback_backup_timer_.stop();

    // Handle audio
    AudioManager::instance()->StopOutput();
    AudioMonitor::StopOnAll();
    prequeued_audio_.clear();
    disconnect(AudioManager::instance(), &AudioManager::OutputNotify, this, &ViewerWidget::QueueNextAudioBuffer);
    qDeleteAll(audio_playback_queue_);
    audio_playback_queue_.clear();
    UpdateAudioProcessor();

    foreach (ViewerWidget* viewer, instances_) {
      viewer->auto_cacher_->SetThumbnailsPaused(false);
    }

    UpdateTextureFromNode();

    RenderManager::instance()->SetAggressiveGarbageCollection(false);
  }

  prequeuing_video_ = false;
  prequeuing_audio_ = 0;
  dry_run_watchers_.clear();

  // Reset screen timeout timer
  PreventSleep(false);
}

void ViewerWidget::PushScrubbedAudio()
{
  if (!IsPlaying() && GetConnectedNode() && OLIVE_CONFIG("AudioScrubbing").toBool() && enable_audio_scrubbing_) {
    if (ignore_scrub_ > 0) {
      ignore_scrub_--;
    }

    if (ignore_scrub_ == 0) {
      // Get audio src device from renderer
      const AudioParams& params = GetConnectedNode()->GetAudioParams();

      if (params.is_valid()) {
        // NOTE: Hardcoded scrubbing interval (20ms)
        rational interval = rational(20, 1000);

        RenderTicketWatcher *watcher = new RenderTicketWatcher();
        connect(watcher, &RenderTicketWatcher::Finished, this, &ViewerWidget::ReceivedAudioBufferForScrubbing);
        audio_scrub_watchers_.push_back(watcher);
        watcher->SetTicket(auto_cacher_->GetRangeOfAudio(TimeRange(GetConnectedNode()->GetPlayhead(), GetConnectedNode()->GetPlayhead() + interval)));
      }
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
    return GetConnectedNode()->video_frame_cache()->GetValidCacheFilename(time);
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

void ViewerWidget::SetDisplayImage(RenderTicketPtr ticket)
{
  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    QVariant push;
    if (ticket) {
      if (dynamic_cast<MulticamDisplay*>(dw)) {
        push = ticket->property("multicam_output");
      } else {
        push = ticket->Get();
      }
    }
    dw->SetImage(push);
  }
}

RenderTicketWatcher *ViewerWidget::RequestNextFrameForQueue(bool increment)
{
  RenderTicketWatcher *watcher = nullptr;

  rational next_time = Timecode::timestamp_to_time(playback_queue_next_frame_,
                                                   timebase());

  if (FrameExistsAtTime(next_time) || ViewerMightBeAStill()) {
    if (increment) {
      playback_queue_next_frame_ += playback_speed_;
    }

    watcher = new RenderTicketWatcher();
    watcher->setProperty("time", QVariant::fromValue(next_time));
    DetectMulticamNode(next_time);
    connect(watcher, &RenderTicketWatcher::Finished, this, &ViewerWidget::RendererGeneratedFrameForQueue);
    queue_watchers_.append(watcher);
    watcher->SetTicket(GetFrame(next_time));
  }

  return watcher;
}

RenderTicketPtr ViewerWidget::GetFrame(const rational &t)
{
  QString cache_fn = GetConnectedNode()->video_frame_cache()->GetValidCacheFilename(t);

  if (!QFileInfo::exists(cache_fn)) {
    // Frame hasn't been cached, start render job
    return GetSingleFrame(t);
  } else {
    // Frame has been cached, grab the frame
    RenderTicketPtr ticket = std::make_shared<RenderTicket>();
    ticket->setProperty("time", QVariant::fromValue(t));
    QtConcurrent::run(static_cast<void(*)(RenderTicketPtr, const QString &, const QUuid &, const int64_t &)>(ViewerWidget::DecodeCachedImage), ticket, GetConnectedNode()->video_frame_cache()->GetCacheDirectory(), GetConnectedNode()->video_frame_cache()->GetUuid(), Timecode::time_to_timestamp(t, timebase(), Timecode::kFloor));
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
    QString error;
    if (!AudioManager::instance()->PushToOutput(audio_processor_.to(), prequeued_audio_, &error)) {
      QMessageBox::critical(this, tr("Audio Error"), tr("Failed to start audio: %1\n\n"
                                                        "Please check your audio preferences and try again.").arg(error));
    }
    prequeued_audio_.clear();

    AudioMonitor::StartWaveformOnAll(GetConnectedNode()->GetConnectedWaveform(),
                                     GetConnectedNode()->GetPlayhead(), playback_speed_);
  }

  display_widget_->ResetFPSTimer();

  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->Play(playback_start_time, playback_speed_, timebase(), IsVideoVisible());
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

  int remaining_frames = (end_ts - GetTimestamp() - 1) / playback_speed_;

  // Generate maximum queue
  int max_frames = qCeil(kVideoPlaybackInterval.toDouble() / timebase().toDouble());

  return qMin(max_frames, remaining_frames);
}

void ViewerWidget::ContextMenuSetFullScreen(QAction *action)
{
  SetFullScreen(QGuiApplication::screens().at(action->data().toInt()));
}

void ViewerWidget::ContextMenuSetPlaybackRes(QAction *action)
{
  int div = action->data().toInt();

  auto vp = GetConnectedNode()->GetVideoParams();
  vp.set_divider(div);

  auto c = new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(NodeInput(GetConnectedNode(), ViewerOutput::kVideoParamsInput, 0)), QVariant::fromValue(vp));
  Core::instance()->undo_stack()->push(c);
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

      SetDisplayImage(ticket->GetTicket());
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
          QVariant push;
          if (dynamic_cast<MulticamDisplay*>(dw)) {
            push = watcher->GetTicket()->property("multicam_output");
          } else {
            push = frame;
          }

          dw->queue()->AppendTimewise({ts, push}, playback_speed_);
        }

        if (prequeuing_video_) {
          prequeue_count_++;

          if (prequeue_count_ == prequeue_length_) {
            prequeuing_video_ = false;
            FinishPlayPreprocess();
          } else {
            // This call was mostly necessary to keep the threads busy between prequeue and playback.
            // If we only have a single render thread, it's no longer necessary.
            //RequestNextFrameForQueue();
          }
        }
      }
    }
  }

  if (first_requeue_watcher_ == watcher) {
    first_requeue_watcher_ = nullptr;
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
        Menu* ocio_colorspace_menu = context_menu_widget_->GetColorSpaceMenu(&menu);
        menu.addMenu(ocio_colorspace_menu);
      }

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

      zoom_menu->addAction(tr("Fit"))->setData(0);
      for (int i=0;i<ViewerSizer::kZoomLevelCount;i++) {
        int z = ViewerSizer::kZoomLevels[i];
        zoom_menu->addAction(tr("%1%").arg(z))->setData(z);
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
      // Playback Resolution Menu
      Menu *playback_res_menu = new Menu(tr("Playback Resolution"), &menu);
      menu.addMenu(playback_res_menu);

      for (int d : VideoParams::kSupportedDividers) {
        playback_res_menu->AddActionWithData(VideoParams::GetNameForDivider(d), d, GetConnectedNode()->GetVideoParams().divider());
      }

      connect(playback_res_menu, &QMenu::triggered, this, &ViewerWidget::ContextMenuSetPlaybackRes);
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

    /* TEMP: Hide sequence cache options. Want to see if clip caching supersedes it.
    {
      Menu* cache_menu = new Menu(tr("Cache"), &menu);
      menu.addMenu(cache_menu);

      // Cache Entire Sequence
      QAction* cache_entire_sequence = cache_menu->addAction(tr("Cache Entire Sequence"));
      connect(cache_entire_sequence, &QAction::triggered, this, &ViewerWidget::CacheEntireSequence);

      // Cache In/Out Sequence
      QAction* cache_inout_sequence = cache_menu->addAction(tr("Cache Sequence In/Out"));
      connect(cache_inout_sequence, &QAction::triggered, this, &ViewerWidget::CacheSequenceInOut);
    }*/

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
    stop_playback_on_last_frame->setChecked(OLIVE_CONFIG("StopPlaybackOnLastFrame").toBool());
    connect(stop_playback_on_last_frame, &QAction::triggered, this, [](bool e){
      OLIVE_CONFIG("StopPlaybackOnLastFrame") = e;
    });

    menu.addSeparator();
  }

  {
    auto waveform_menu = new Menu(tr("Audio Waveform"), &menu);
    menu.addMenu(waveform_menu);

    waveform_menu->AddActionWithData(tr("Automatically Show/Hide"), kWFAutomatic, waveform_mode_);
    waveform_menu->AddActionWithData(tr("Show Waveform Only"), kWFWaveformOnly, waveform_mode_);
    waveform_menu->AddActionWithData(tr("Show Both Viewer And Waveform"), kWFViewerAndWaveform, waveform_mode_);

    connect(waveform_menu, &Menu::triggered, this, &ViewerWidget::UpdateWaveformModeFromMenu);
  }

  {
    QAction* show_fps_action = menu.addAction(tr("Show FPS"));
    show_fps_action->setCheckable(true);
    show_fps_action->setChecked(display_widget_->GetShowFPS());
    connect(show_fps_action, &QAction::triggered, display_widget_, &ViewerDisplayWidget::SetShowFPS);
  }

  if (context_menu_widget_ == display_widget_) {
    auto subtitle_menu = new Menu(tr("Subtitles"), &menu);
    menu.addMenu(subtitle_menu);

    QAction* show_subtitles_action = subtitle_menu->addAction(tr("Show Subtitles"));
    show_subtitles_action->setCheckable(true);
    show_subtitles_action->setChecked(display_widget_->GetShowSubtitles());
    connect(show_subtitles_action, &QAction::triggered, display_widget_, &ViewerDisplayWidget::SetShowSubtitles);

    subtitle_menu->addSeparator();

    auto subtitle_font_properties = subtitle_menu->addAction(tr("Subtitle Properties"));
    connect(subtitle_font_properties, &QAction::triggered, this, &ViewerWidget::ShowSubtitleProperties);

    auto subtitle_antialias = subtitle_menu->addAction(tr("Use Anti-aliasing"));
    subtitle_antialias->setCheckable(true);
    subtitle_antialias->setChecked(OLIVE_CONFIG("AntialiasSubtitles").toBool());
    connect(subtitle_antialias, &QAction::triggered, this, [this](bool e){
      OLIVE_CONFIG("AntialiasSubtitles") = e;
      display_widget_->update();
    });
  }

  menu.addSeparator();

  auto save_frame_as_image = menu.addAction(tr("Save Frame As Image"));
  connect(save_frame_as_image, &QAction::triggered, this, &ViewerWidget::SaveFrameAsImage);

  menu.exec(static_cast<QWidget*>(sender())->mapToGlobal(pos));
}

void ViewerWidget::Play(bool in_to_out_only)
{
  if (in_to_out_only) {
    if (GetConnectedNode()
        && GetConnectedNode()->GetWorkArea()->enabled()) {
      // Jump to in point
      GetConnectedNode()->SetPlayhead(GetConnectedNode()->GetWorkArea()->in());
    } else {
      in_to_out_only = false;
    }
  } else if (record_armed_) {
    DisarmRecording();

    if (GetConnectedNode()->project()->filename().isEmpty()) {
      QMessageBox::critical(this, tr("Audio Recording"), tr("Project must be saved before you can record audio."));
      return;
    }

    QDir audio_path(QFileInfo(GetConnectedNode()->project()->filename()).dir().filePath(tr("audio")));
    if (!audio_path.exists()) {
      audio_path.mkpath(QStringLiteral("."));
    }

    recording_filename_ = audio_path.filePath(QStringLiteral("%1.%2").arg(
                                                QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss"),
                                                ExportFormat::GetExtension(static_cast<ExportFormat::Format>(OLIVE_CONFIG("AudioRecordingFormat").toInt())))
                                              );

    AudioParams ap(OLIVE_CONFIG("AudioRecordingSampleRate").toInt(), OLIVE_CONFIG("AudioRecordingChannelLayout").toULongLong(), static_cast<AudioParams::Format>(OLIVE_CONFIG("AudioRecordingSampleFormat").toInt()));

    EncodingParams encode_param;
    encode_param.EnableAudio(ap, static_cast<ExportCodec::Codec>(OLIVE_CONFIG("AudioRecordingCodec").toInt()));
    encode_param.SetFilename(recording_filename_);
    encode_param.set_audio_bit_rate(OLIVE_CONFIG("AudioRecordingBitRate").toInt() * 1000);

    QString error;
    if (AudioManager::instance()->StartRecording(encode_param, &error)) {
      recording_ = true;
      controls_->SetPauseButtonRecordingState(true);
      recording_callback_->EnableRecordingOverlay(TimelineCoordinate(recording_range_.in(), recording_track_));
    } else {
      QMessageBox::critical(this, tr("Audio Recording"), tr("Failed to start audio recording: %1").arg(error));
      return;
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

  controls_->SetTime(GetConnectedNode() ? GetConnectedNode()->GetPlayhead() : 0);
  LengthChangedSlot(GetConnectedNode() ? GetConnectedNode()->GetLength() : 0);
}

void ViewerWidget::PlaybackTimerUpdate()
{
  Q_ASSERT(playback_speed_ != 0);

  rational current_time = Timecode::timestamp_to_time(display_widget_->timer()->GetTimestampNow(), timebase());

  rational min_time, max_time;

  if (recording_ && recording_range_.out() != recording_range_.in()) {

    // Limit recording range if applicable
    min_time = recording_range_.in();
    max_time = recording_range_.out();

  } else if (play_in_to_out_only_ && GetConnectedNode()->GetWorkArea()->enabled()) {

    // If "play in to out" is enabled or we're looping AND we have a workarea, only play the workarea
    min_time = GetConnectedNode()->GetWorkArea()->in();
    max_time = GetConnectedNode()->GetWorkArea()->out();

  } else {

    // Otherwise set the bounds to the range of the sequence
    min_time = 0;
    max_time = GetConnectedNode()->GetLength();

  }

  // If we're stopping playback on the last frame rather than after it, subtract our max time
  // by one timebase unit
  if (OLIVE_CONFIG("StopPlaybackOnLastFrame").toBool()) {
    max_time = qMax(min_time, max_time - timebase());
  }

  rational time_to_set;
  bool end_of_line = false;
  bool play_after_pause = false;

  if ((!recording_ || recording_range_.out() != recording_range_.in())
      && ((playback_speed_ < 0 && current_time <= min_time)
          || (playback_speed_ > 0 && current_time >= max_time))) {

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

    if (OLIVE_CONFIG("Loop").toBool() && !recording_) {

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
  GetConnectedNode()->SetPlayhead(time_to_set);
  time_changed_from_timer_ = false;
  if (end_of_line) {
    // Cache the current speed
    int current_speed = playback_speed_;

    PauseInternal();
    if (play_after_pause) {
      PlayInternal(current_speed, play_in_to_out_only_);
    }
  }

  if (IsPlaying() && IsVideoVisible()) {
    while ((int(display_widget_->queue()->size()) + queue_watchers_.size()) < DeterminePlaybackQueueSize()) {
      if (!RequestNextFrameForQueue()) {
        // Prevent infinite loop
        break;
      }
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

    if (GetConnectedNode() && length < last_length_ && GetConnectedNode()->GetPlayhead() >= length) {
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
  AudioParams ap = GetConnectedNode()->GetAudioParams();

  UpdateAudioProcessor();

  foreach (ViewerDisplayWidget *dw, playback_devices_) {
    dw->SetAudioParams(ap);
  }
}

void ViewerWidget::SetZoomFromMenu(QAction *action)
{
  sizer_->SetZoom(action->data().toInt());
}

void ViewerWidget::ViewerInvalidatedVideoRange(const TimeRange &range)
{
  // If our current frame is within this range, we need to update
  if (!IsPlaying() && GetConnectedNode()->GetPlayhead() >= range.in() && (GetConnectedNode()->GetPlayhead() < range.out() || range.in() == range.out())) {
    QMetaObject::invokeMethod(this, &ViewerWidget::UpdateTextureFromNode, Qt::QueuedConnection);
  }
}

void ViewerWidget::UpdateWaveformModeFromMenu(QAction *a)
{
  SetWaveformMode(static_cast<WaveformMode>(a->data().toInt()));
}

void ViewerWidget::DragEntered(QDragEnterEvent* event)
{
  if (event->mimeData()->formats().contains(Project::kItemMimeType)) {
    event->accept();
  }
}

void ViewerWidget::Dropped(QDropEvent *event)
{
  QByteArray mimedata = event->mimeData()->data(Project::kItemMimeType);
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
