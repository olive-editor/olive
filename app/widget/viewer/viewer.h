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

#ifndef VIEWER_WIDGET_H
#define VIEWER_WIDGET_H

#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QTimer>
#include <QWidget>

#include "audio/audioprocessor.h"
#include "audiowaveformview.h"
#include "node/output/viewer/viewer.h"
#include "render/previewaudiodevice.h"
#include "render/previewautocacher.h"
#include "viewerdisplay.h"
#include "viewersizer.h"
#include "viewerwindow.h"
#include "widget/playbackcontrols/playbackcontrols.h"
#include "widget/timebased/timebasedwidget.h"
#include "widget/timelinewidget/timelinewidget.h"

namespace olive {

class MulticamWidget;

/**
 * @brief An OpenGL-based viewer widget with playback controls (a PlaybackControls widget).
 */
class ViewerWidget : public TimeBasedWidget
{
  Q_OBJECT
public:
  enum WaveformMode {
    kWFAutomatic,
    kWFViewerOnly,
    kWFWaveformOnly,
    kWFViewerAndWaveform
  };

  ViewerWidget(QWidget* parent = nullptr) :
    ViewerWidget(new ViewerDisplayWidget(), parent)
  {}

  virtual ~ViewerWidget() override;

  void SetPlaybackControlsEnabled(bool enabled);

  void SetTimeRulerEnabled(bool enabled);

  void TogglePlayPause();

  bool IsPlaying() const;

  /**
   * @brief Enable or disable the color management menu
   *
   * While the Viewer is _always_ color managed, In some contexts, the color management may be controlled from an
   * external UI making the menu unnecessary.
   */
  void SetColorMenuEnabled(bool enabled);

  void SetMatrix(const QMatrix4x4& mat);

  /**
   * @brief Creates a ViewerWindow widget and places it full screen on another screen
   *
   * If `screen` is nullptr, the screen will be automatically selected as whichever one contains the mouse cursor.
   */
  void SetFullScreen(QScreen* screen = nullptr);

  ColorManager* color_manager() const
  {
    return display_widget_->color_manager();
  }

  void SetGizmos(Node* node);

  void StartCapture(TimelineWidget *source, const TimeRange &time, const Track::Reference &track);

  void SetAudioScrubbingEnabled(bool e)
  {
    enable_audio_scrubbing_ = e;
  }

  PreviewAutoCacher *GetCacher() const { return auto_cacher_; }

  void AddPlaybackDevice(ViewerDisplayWidget *vw)
  {
    playback_devices_.push_back(vw);
  }

  void SetTimelineSelectedBlocks(const QVector<Block*> &b)
  {
    timeline_selected_blocks_ = b;

    if (!IsPlaying()) {
      // If is playing, this will happen by the next frame automatically
      DetectMulticamNodeNow();
      UpdateTextureFromNode();
    }
  }

  void SetNodeViewSelections(const QVector<Node*> &n)
  {
    node_view_selected_ = n;

    if (!IsPlaying()) {
      // If is playing, this will happen by the next frame automatically
      DetectMulticamNodeNow();
      UpdateTextureFromNode();
    }
  }

  void ConnectMulticamWidget(MulticamWidget *p);

public slots:
  void Play(bool in_to_out_only);

  void Play();

  void Pause();

  void ShuttleLeft();

  void ShuttleStop();

  void ShuttleRight();

  void SetColorTransform(const ColorTransform& transform);

  /**
   * @brief Wrapper for ViewerGLWidget::SetSignalCursorColorEnabled()
   */
  void SetSignalCursorColorEnabled(bool e);

  void CacheEntireSequence();

  void CacheSequenceInOut();

  void SetViewerResolution(int width, int height);

  void SetViewerPixelAspect(const rational& ratio);

  void UpdateTextureFromNode();

  void RequestStartEditingText()
  {
    display_widget_->RequestStartEditingText();
  }

signals:
  /**
   * @brief Wrapper for ViewerGLWidget::CursorColor()
   */
  void CursorColor(const Color& reference, const Color& display);

  /**
   * @brief Signal emitted when a new frame is loaded
   */
  void TextureChanged(TexturePtr t);

  /**
   * @brief Wrapper for ViewerGLWidget::ColorProcessorChanged()
   */
  void ColorProcessorChanged(ColorProcessorPtr processor);

  /**
   * @brief Wrapper for ViewerGLWidget::ColorManagerChanged()
   */
  void ColorManagerChanged(ColorManager* color_manager);

protected:
  ViewerWidget(ViewerDisplayWidget *display, QWidget* parent = nullptr);

  virtual void TimebaseChangedEvent(const rational &) override;
  virtual void TimeChangedEvent(const rational &time) override;

  virtual void ConnectNodeEvent(ViewerOutput *) override;
  virtual void DisconnectNodeEvent(ViewerOutput *) override;
  virtual void ConnectedNodeChangeEvent(ViewerOutput *) override;
  virtual void ConnectedWorkAreaChangeEvent(TimelineWorkArea *) override;
  virtual void ConnectedMarkersChangeEvent(TimelineMarkerList *) override;

  virtual void ScaleChangedEvent(const double& s) override;

  virtual void resizeEvent(QResizeEvent *event) override;

  PlaybackControls* controls_;

  ViewerDisplayWidget* display_widget() const
  {
    return display_widget_;
  }

  void IgnoreNextScrubEvent()
  {
    ignore_scrub_++;
  }

  virtual RenderTicketPtr GetSingleFrame(const rational &t, bool dry = false)
  {
    return auto_cacher_->GetSingleFrame(t, dry);
  }

  PreviewAutoCacher *auto_cacher() const { return auto_cacher_; }

private:
  int64_t GetTimestamp() const
  {
    return Timecode::time_to_timestamp(GetConnectedNode()->GetPlayhead(), timebase(), Timecode::kFloor);
  }

  void UpdateTimeInternal(int64_t i);

  void PlayInternal(int speed, bool in_to_out_only);

  void PauseInternal();

  void PushScrubbedAudio();

  void UpdateMinimumScale();

  void SetColorTransform(const ColorTransform& transform, ViewerDisplayWidget* sender);

  QString GetCachedFilenameFromTime(const rational& time);

  bool FrameExistsAtTime(const rational& time);

  bool ViewerMightBeAStill();

  void SetDisplayImage(RenderTicketPtr ticket);

  RenderTicketWatcher *RequestNextFrameForQueue(bool increment = true);

  RenderTicketPtr GetFrame(const rational& t);

  void FinishPlayPreprocess();

  int DeterminePlaybackQueueSize();

  static FramePtr DecodeCachedImage(const QString &cache_path, const QUuid &cache_id, const int64_t& time);

  static void DecodeCachedImage(RenderTicketPtr ticket, const QString &cache_path, const QUuid &cache_id, const int64_t &time);

  bool ShouldForceWaveform() const;

  void SetEmptyImage();

  void UpdateAutoCacher();

  void DecrementPrequeuedAudio();

  void ArmForRecording();

  void DisarmRecording();

  void CloseAudioProcessor();

  void SetWaveformMode(WaveformMode wf);

  void DetectMulticamNode(const rational &time);

  bool IsVideoVisible() const;

  ViewerSizer* sizer_;

  int playback_speed_;

  rational last_time_;

  bool color_menu_enabled_;

  bool time_changed_from_timer_;

  bool play_in_to_out_only_;

  AudioWaveformView* waveform_view_;

  QHash<QScreen*, ViewerWindow*> windows_;

  ViewerDisplayWidget* display_widget_;

  ViewerDisplayWidget* context_menu_widget_;

  QTimer playback_backup_timer_;

  int64_t playback_queue_next_frame_;
  int64_t dry_run_next_frame_;
  QVector<ViewerDisplayWidget*> playback_devices_;

  bool prequeuing_video_;
  int prequeuing_audio_;

  QList<RenderTicketWatcher*> nonqueue_watchers_;

  rational last_length_;

  int prequeue_length_;
  int prequeue_count_;

  PreviewAutoCacher *auto_cacher_;

  QVector<RenderTicketWatcher*> queue_watchers_;

  std::list<RenderTicketWatcher*> audio_playback_queue_;
  rational audio_playback_queue_time_;
  AudioProcessor audio_processor_;
  QByteArray prequeued_audio_;
  static const rational kAudioPlaybackInterval;

  static QVector<ViewerWidget*> instances_;

  std::list<RenderTicketWatcher*> audio_scrub_watchers_;

  bool record_armed_;
  bool recording_;
  TimelineWidget *recording_callback_;
  TimeRange recording_range_;
  Track::Reference recording_track_;
  QString recording_filename_;

  qint64 queue_starved_start_;
  RenderTicketWatcher *first_requeue_watcher_;

  bool enable_audio_scrubbing_;

  WaveformMode waveform_mode_;

  QVector<RenderTicketWatcher*> dry_run_watchers_;

  int ignore_scrub_;

  QVector<Block*> timeline_selected_blocks_;
  QVector<Node*> node_view_selected_;

  MulticamWidget *multicam_panel_;

private slots:
  void PlaybackTimerUpdate();

  void LengthChangedSlot(const rational& length);

  void InterlacingChangedSlot(VideoParams::Interlacing interlacing);

  void UpdateRendererVideoParameters();

  void UpdateRendererAudioParameters();

  void ShowContextMenu(const QPoint& pos);

  void SetZoomFromMenu(QAction* action);

  void UpdateWaveformViewFromMode();

  void ContextMenuSetFullScreen(QAction* action);

  void ContextMenuSetPlaybackRes(QAction* action);

  void ContextMenuDisableSafeMargins();

  void ContextMenuSetSafeMargins();

  void ContextMenuSetCustomSafeMargins();

  void WindowAboutToClose();

  void RendererGeneratedFrame();

  void RendererGeneratedFrameForQueue();

  void ViewerInvalidatedVideoRange(const olive::TimeRange &range);

  void UpdateWaveformModeFromMenu(QAction *a);

  void DragEntered(QDragEnterEvent* event);

  void Dropped(QDropEvent* event);

  void QueueNextAudioBuffer();

  void ReceivedAudioBufferForPlayback();

  void ReceivedAudioBufferForScrubbing();

  void QueueStarved();
  void QueueNoLongerStarved();

  void ForceRequeueFromCurrentTime();

  void UpdateAudioProcessor();

  void CreateAddableAt(const QRectF &f);

  void HandleFirstRequeueDestroy();

  void ShowSubtitleProperties();

  void DryRunFinished();

  void RequestNextDryRun();

  void SaveFrameAsImage();

  void DetectMulticamNodeNow();

};

}

#endif // VIEWER_WIDGET_H
