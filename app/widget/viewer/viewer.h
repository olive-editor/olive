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

#ifndef VIEWER_WIDGET_H
#define VIEWER_WIDGET_H

#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QTimer>
#include <QWidget>

#include "audio/packedprocessor.h"
#include "audio/tempoprocessor.h"
#include "audiowaveformview.h"
#include "common/rational.h"
#include "node/output/viewer/viewer.h"
#include "render/previewaudiodevice.h"
#include "render/previewautocacher.h"
#include "threading/threadticketwatcher.h"
#include "viewerdisplay.h"
#include "viewerplaybacktimer.h"
#include "viewerqueue.h"
#include "viewersizer.h"
#include "viewerwindow.h"
#include "widget/playbackcontrols/playbackcontrols.h"
#include "widget/timebased/timebasedwidget.h"

namespace olive {

/**
 * @brief An OpenGL-based viewer widget with playback controls (a PlaybackControls widget).
 */
class ViewerWidget : public TimeBasedWidget
{
  Q_OBJECT
public:
  ViewerWidget(QWidget* parent = nullptr);

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
  virtual void TimebaseChangedEvent(const rational &) override;
  virtual void TimeChangedEvent(const rational &time) override;

  virtual void ConnectNodeEvent(ViewerOutput *) override;
  virtual void DisconnectNodeEvent(ViewerOutput *) override;
  virtual void ConnectedNodeChangeEvent(ViewerOutput *) override;

  virtual void ScaleChangedEvent(const double& s) override;

  virtual void resizeEvent(QResizeEvent *event) override;

  PlaybackControls* controls_;

  ViewerDisplayWidget* display_widget() const
  {
    return display_widget_;
  }

private:
  int64_t GetTimestamp() const
  {
    return Timecode::time_to_timestamp(GetTime(), timebase(), Timecode::kFloor);
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

  void SetDisplayImage(QVariant frame, bool main_only = false);

  void RequestNextFrameForQueue(bool prioritize = false, bool increment = true);

  RenderTicketPtr GetFrame(const rational& t, bool prioritize);

  void FinishPlayPreprocess();

  int DeterminePlaybackQueueSize();

  void PopOldestFrameFromPlaybackQueue();

  static FramePtr DecodeCachedImage(const QString &cache_path, const QByteArray &hash, const rational& time);

  static void DecodeCachedImage(RenderTicketPtr ticket, const QString &cache_path, const QByteArray &hash, const rational& time);

  bool ShouldForceWaveform() const;

  void SetEmptyImage();

  void UpdateAutoCacher();

  void ClearVideoAutoCacherQueue();

  void DecrementPrequeuedAudio();

  QStackedWidget* stack_;

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

  ViewerPlaybackTimer playback_timer_;
  QTimer playback_backup_timer_;

  ViewerQueue playback_queue_;
  int64_t playback_queue_next_frame_;

  bool prequeuing_video_;
  int prequeuing_audio_;

  QList<RenderTicketWatcher*> nonqueue_watchers_;

  rational last_length_;

  int prequeue_length_;

  PreviewAutoCacher auto_cacher_;

  QVector<RenderTicketWatcher*> queue_watchers_;

  std::list<RenderTicketWatcher*> audio_playback_queue_;
  rational audio_playback_queue_time_;
  PackedProcessor packed_processor_;
  TempoProcessor tempo_processor_;
  QByteArray prequeued_audio_;
  static const rational kAudioPlaybackInterval;

  static QVector<ViewerWidget*> instances_;

private slots:
  void PlaybackTimerUpdate();

  void LengthChangedSlot(const rational& length);

  void InterlacingChangedSlot(VideoParams::Interlacing interlacing);

  void UpdateRendererVideoParameters();

  void UpdateRendererAudioParameters();

  void ShowContextMenu(const QPoint& pos);

  void SetZoomFromMenu(QAction* action);

  void ViewerShiftedRange(const olive::rational& from, const olive::rational& to);

  void UpdateStack();

  void ContextMenuSetFullScreen(QAction* action);

  void ContextMenuDisableSafeMargins();

  void ContextMenuSetSafeMargins();

  void ContextMenuSetCustomSafeMargins();

  void WindowAboutToClose();

  void RendererGeneratedFrame();

  void RendererGeneratedFrameForQueue();

  void ViewerInvalidatedVideoRange(const olive::TimeRange &range);

  void ManualSwitchToWaveform(bool e);

  void DragEntered(QDragEnterEvent* event);

  void Dropped(QDropEvent* event);

  void QueueNextAudioBuffer();

  void ReceivedAudioBufferForPlayback();

  void ReceivedAudioBufferForScrubbing();

  void ForceRequeueFromCurrentTime();

};

}

#endif // VIEWER_WIDGET_H
