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

#include "common/rational.h"
#include "node/output/viewer/viewer.h"
#include "render/backend/opengl/openglbackend.h"
#include "render/backend/opengl/opengltexture.h"
#include "render/backend/audio/audiobackend.h"
#include "viewerglwidget.h"
#include "viewersizer.h"
#include "waveformview.h"
#include "widget/playbackcontrols/playbackcontrols.h"
#include "widget/timebased/timebased.h"

/**
 * @brief An OpenGL-based viewer widget with playback controls (a PlaybackControls widget).
 */
class ViewerWidget : public TimeBasedWidget
{
  Q_OBJECT
public:
  ViewerWidget(QWidget* parent = nullptr);

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

  VideoRenderBackend* video_renderer() const;

public slots:
  void Play();

  void Pause();

  void ShuttleLeft();

  void ShuttleStop();

  void ShuttleRight();

  void SetOCIOParameters(const QString& display, const QString& view, const QString& look);

  /**
   * @brief Externally set the OCIO display to use
   *
   * This value must be a valid display in the current OCIO configuration.
   */
  void SetOCIODisplay(const QString& display);

  /**
   * @brief Externally set the OCIO view to use
   *
   * This value must be a valid display in the current OCIO configuration.
   */
  void SetOCIOView(const QString& view);

  /**
   * @brief Externally set the OCIO look to use (use empty string if none)
   *
   * This value must be a valid display in the current OCIO configuration.
   */
  void SetOCIOLook(const QString& look);

protected:
  virtual void TimebaseChangedEvent(const rational &) override;
  virtual void TimeChangedEvent(const int64_t &) override;

  virtual void ConnectNodeInternal(ViewerOutput *) override;
  virtual void DisconnectNodeInternal(ViewerOutput *) override;
  virtual void ConnectedNodeChanged(ViewerOutput*n) override;

  virtual void ScaleChangedEvent(const double& s) override;

  virtual void resizeEvent(QResizeEvent *event) override;

  OpenGLBackend* video_renderer_;
  AudioBackend* audio_renderer_;

  ViewerGLWidget* gl_widget_;

private:
  void UpdateTimeInternal(int64_t i);

  void UpdateTextureFromNode(const rational &time);

  void PlayInternal(int speed);

  void PushScrubbedAudio();

  int CalculateDivider();

  void UpdateMinimumScale();

  QStackedWidget* stack_;

  ViewerSizer* sizer_;

  PlaybackControls* controls_;

  qint64 start_msec_;
  int64_t start_timestamp_;

  int playback_speed_;

  qint64 frame_cache_job_time_;

  int64_t last_time_;

  bool color_menu_enabled_;

  int divider_;

  ColorManager* override_color_manager_;

  bool time_changed_from_timer_;

  WaveformView* waveform_view_;

private slots:
  void PlaybackTimerUpdate();

  void RendererCachedTime(const rational& time, qint64 job_time);

  void SizeChangedSlot(int width, int height);

  void LengthChangedSlot(const rational& length);

  void UpdateRendererParameters();

  void ShowContextMenu(const QPoint& pos);

  /**
   * @brief Slot called whenever this viewer's OCIO display setting has changed
   */
  void ColorDisplayChanged(QAction* action);

  /**
   * @brief Slot called whenever this viewer's OCIO view setting has changed
   */
  void ColorViewChanged(QAction* action);

  /**
   * @brief Slot called whenever this viewer's OCIO look setting has changed
   */
  void ColorLookChanged(QAction* action);

  void SetDividerFromMenu(QAction* action);

  void SetZoomFromMenu(QAction* action);

  void InvalidateVisible();

  void UpdateStack();

};

#endif // VIEWER_WIDGET_H
