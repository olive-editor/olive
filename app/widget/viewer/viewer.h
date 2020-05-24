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

#ifndef VIEWER_WIDGET_H
#define VIEWER_WIDGET_H

#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QTimer>
#include <QWidget>

#include "audiowaveformview.h"
#include "common/rational.h"
#include "node/output/viewer/viewer.h"
#include "panel/scope/scope.h"
#include "render/backend/opengl/openglbackend.h"
#include "task/cache/cache.h"
#include "viewerdisplay.h"
#include "viewerplaybacktimer.h"
#include "viewerqueue.h"
#include "viewersizer.h"
#include "viewerwindow.h"
#include "widget/playbackcontrols/playbackcontrols.h"
#include "widget/timebased/timebased.h"

OLIVE_NAMESPACE_ENTER

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

  void ConnectViewerNode(ViewerOutput* node, ColorManager *color_manager = nullptr);

  /**
   * @brief Enable or disable the color management menu
   *
   * While the Viewer is _always_ color managed, In some contexts, the color management may be controlled from an
   * external UI making the menu unnecessary.
   */
  void SetColorMenuEnabled(bool enabled);

  void SetOverrideSize(int width, int height);

  void SetMatrix(const QMatrix4x4& mat);

  /**
   * @brief Creates a ViewerWindow widget and places it full screen on another screen
   *
   * If `screen` is nullptr, the screen will be automatically selected as whichever one contains the mouse cursor.
   */
  void SetFullScreen(QScreen* screen = nullptr);

  void ForceUpdate();

  RenderBackend* renderer() const
  {
    return renderer_;
  }

  ColorManager* color_manager() const
  {
    return display_widget_->color_manager();
  }

  void SetGizmos(Node* node);

  static void StopAllBackgroundCacheTasks(bool wait);
  static void SetBackgroundCacheTask(CacheTask* t);

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

signals:
  /**
   * @brief Wrapper for ViewerGLWidget::CursorColor()
   */
  void CursorColor(const Color& reference, const Color& display);

  /**
   * @brief Signal emitted when a new frame is loaded
   */
  void LoadedBuffer(Frame* load_buffer);

  /**
   * @brief Request a scope panel
   *
   * As a widget, we don't handle panels, but a parent panel may pick this signal up.
   */
  void RequestScopePanel(ScopePanel::Type type);

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
  virtual void TimeChangedEvent(const int64_t &) override;

  virtual void ConnectNodeInternal(ViewerOutput *) override;
  virtual void DisconnectNodeInternal(ViewerOutput *) override;
  virtual void ConnectedNodeChanged(ViewerOutput*n) override;

  virtual void ScaleChangedEvent(const double& s) override;

  virtual void resizeEvent(QResizeEvent *event) override;

  PlaybackControls* controls_;

  ViewerDisplayWidget* display_widget() const;

private:
  void UpdateTimeInternal(int64_t i);

  void UpdateTextureFromNode(const rational &time);

  void PlayInternal(int speed, bool in_to_out_only);

  void PauseInternal();

  void PushScrubbedAudio();

  int CalculateDivider();

  void UpdateMinimumScale();

  void SetColorTransform(const ColorTransform& transform, ViewerDisplayWidget* sender);

  void FillPlaybackQueue();

  QString GetCachedFilenameFromTime(const rational& time);

  bool FrameExistsAtTime(const rational& time);

  void SetDisplayImage(FramePtr frame, bool main_only);

  void RequestNextFrameForQueue();

  PixelFormat::Format GetCurrentPixelFormat() const;

  QFuture<FramePtr> GetFrame(const rational& t, bool clear_render_queue, bool block_update);

  void FinishPlayPreprocess();

  VideoRenderingParams GenerateVideoParams() const;
  AudioRenderingParams GenerateAudioParams() const;

  QStackedWidget* stack_;

  ViewerSizer* sizer_;

  QAtomicInt playback_speed_;

  qint64 frame_cache_job_time_;

  int64_t last_time_;

  bool color_menu_enabled_;

  int divider_;

  ColorManager* override_color_manager_;

  bool time_changed_from_timer_;

  bool play_in_to_out_only_;

  bool playback_is_audio_only_;

  AudioWaveformView* waveform_view_;

  QList<ViewerWindow*> windows_;

  ViewerDisplayWidget* display_widget_;

  ViewerDisplayWidget* context_menu_widget_;

  ViewerPlaybackTimer playback_timer_;

  ViewerQueue playback_queue_;
  int64_t playback_queue_next_frame_;

  RenderBackend* renderer_;

  bool prequeuing_;

  QList< QFutureWatcher<FramePtr>* > nonqueue_watchers_;

  QHash<QFutureWatcher<QByteArray>*, rational> hash_watchers_;

  QTimer cache_wait_timer_;

  bool busy_;

  CacheTask* our_cache_background_task_;

  static CacheTask* cache_background_task_;

  static int busy_viewers_;

private slots:
  void PlaybackTimerUpdate();

  void SizeChangedSlot(int width, int height);

  void LengthChangedSlot(const rational& length);

  void UpdateRendererParameters();

  void ShowContextMenu(const QPoint& pos);

  void SetDividerFromMenu(QAction* action);

  void SetZoomFromMenu(QAction* action);

  void ViewerInvalidatedRange(const OLIVE_NAMESPACE::TimeRange &range);

  void UpdateStack();

  void ContextMenuSetFullScreen(QAction* action);

  void ContextMenuDisableSafeMargins();

  void ContextMenuSetSafeMargins();

  void ContextMenuSetCustomSafeMargins();

  void WindowAboutToClose();

  void ContextMenuScopeTriggered(QAction* action);

  void RendererGeneratedFrame();

  void RendererGeneratedFrameForQueue();

  void HashGenerated();

  void StartBackgroundCaching();

  void BackgroundCacheFinished(Task *t);

};

OLIVE_NAMESPACE_EXIT

#endif // VIEWER_WIDGET_H
