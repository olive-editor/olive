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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "panel/panelmanager.h"
#include "panel/audiomonitor/audiomonitor.h"
#include "panel/curve/curve.h"
#include "panel/node/node.h"
#include "panel/param/param.h"
#include "panel/project/project.h"
#include "panel/taskmanager/taskmanager.h"
#include "panel/timeline/timeline.h"
#include "panel/tool/tool.h"
#include "panel/footageviewer/footageviewer.h"
#include "panel/sequenceviewer/sequenceviewer.h"
#include "panel/pixelsampler/pixelsamplerpanel.h"
#include "project/project.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief Olive's main window responsible for docking widgets and the main menu bar.
 */
class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow(QWidget *parent = nullptr);

  void OpenSequence(Sequence* sequence);

  void CloseSequence(Sequence* sequence);

public slots:
  void ProjectOpen(Project *p);
  void SetFullscreen(bool fullscreen);
  void ToggleMaximizedPanel();

  void SetDefaultLayout();

protected:
  virtual void closeEvent(QCloseEvent* e) override;

private:
  TimelinePanel* AppendTimelinePanel();

  void TimelineFocused(TimelinePanel *panel);

  QByteArray premaximized_state_;

  // Standard panels
  NodePanel* node_panel_;
  ParamPanel* param_panel_;
  SequenceViewerPanel* sequence_viewer_panel_;
  FootageViewerPanel* footage_viewer_panel_;
  ProjectPanel* project_panel_;
  ToolPanel* tool_panel_;
  QList<TimelinePanel*> timeline_panels_;
  AudioMonitorPanel* audio_monitor_panel_;
  TaskManagerPanel* task_man_panel_;
  CurvePanel* curve_panel_;
  PixelSamplerPanel* pixel_sampler_panel_;

private slots:
  void FocusedPanelChanged(PanelWidget* panel);

  void UpdateTitle();

};

OLIVE_NAMESPACE_EXIT

#endif
