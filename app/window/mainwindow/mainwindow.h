/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "mainwindowlayoutinfo.h"
#include "panel/panelmanager.h"
#include "panel/audiomonitor/audiomonitor.h"
#include "panel/curve/curve.h"
#include "panel/node/node.h"
#include "panel/param/param.h"
#include "panel/project/project.h"
#include "panel/scope/scope.h"
#include "panel/table/table.h"
#include "panel/taskmanager/taskmanager.h"
#include "panel/timeline/timeline.h"
#include "panel/tool/tool.h"
#include "panel/footageviewer/footageviewer.h"
#include "panel/sequenceviewer/sequenceviewer.h"
#include "panel/pixelsampler/pixelsamplerpanel.h"
#include "project/project.h"

#ifdef Q_OS_WINDOWS
#include <shobjidl.h>
#endif

OLIVE_NAMESPACE_ENTER

/**
 * @brief Olive's main window responsible for docking widgets and the main menu bar.
 */
class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  MainWindow(QWidget *parent = nullptr);

  virtual ~MainWindow() override;

  void LoadLayout(const MainWindowLayoutInfo &info);

  MainWindowLayoutInfo SaveLayout() const;

  TimelinePanel *OpenSequence(Sequence* sequence, bool enable_focus = true);

  void CloseSequence(Sequence* sequence);

  bool IsSequenceOpen(Sequence* sequence) const;

  void FolderOpen(Project* p, Item* i, bool floating);

  ScopePanel* AppendScopePanel();

  enum ProgressStatus {
    kProgressNone,
    kProgressShow,
    kProgressError
  };

  /**
   * @brief Where applicable, show progress on an operating system level
   *
   * * For Windows, this is shown as progress in the taskbar.
   * * For macOS, this is shown as progress in the dock.
   */
  void SetApplicationProgressStatus(ProgressStatus status);

  /**
   * @brief If SetApplicationProgressStatus is set to kShowProgress, set the value with this
   *
   * Expects a percentage (0-100 inclusive).
   */
  void SetApplicationProgressValue(int value);

public slots:
  void ProjectOpen(Project *p);

  void ProjectClose(Project* p);

  void SetFullscreen(bool fullscreen);

  void ToggleMaximizedPanel();

  void SetDefaultLayout();

protected:
  virtual void showEvent(QShowEvent* e) override;

  virtual void closeEvent(QCloseEvent* e) override;

#ifdef Q_OS_WINDOWS
  virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

private:
  TimelinePanel* AppendTimelinePanel();

  ProjectPanel* AppendProjectPanel();

  template <typename T>
  T* AppendPanelInternal(QList<T*>& list);

  template <typename T>
  T* AppendFloatingPanelInternal(QList<T*>& list);

  template<typename T>
  void SetUniquePanelID(T* panel, const QList<T*>& list);

  void RemoveTimelinePanel(TimelinePanel *panel);

  void RemoveProjectPanel(ProjectPanel* panel);

  void TimelineFocused(ViewerOutput *viewer);

  QByteArray premaximized_state_;

  // Standard panels
  NodePanel* node_panel_;
  ParamPanel* param_panel_;
  CurvePanel* curve_panel_;
  SequenceViewerPanel* sequence_viewer_panel_;
  FootageViewerPanel* footage_viewer_panel_;
  QList<ProjectPanel*> project_panels_;
  QList<ProjectPanel*> folder_panels_;
  ToolPanel* tool_panel_;
  QList<TimelinePanel*> timeline_panels_;
  AudioMonitorPanel* audio_monitor_panel_;
  TaskManagerPanel* task_man_panel_;
  PixelSamplerPanel* pixel_sampler_panel_;
  QList<ScopePanel*> scope_panels_;
  NodeTablePanel* table_panel_;

#ifdef Q_OS_WINDOWS
  unsigned int taskbar_btn_id_;

  ITaskbarList3* taskbar_interface_;
#endif

private slots:
  void FocusedPanelChanged(PanelWidget* panel);

  void UpdateTitle();

  void TimelineCloseRequested();

  void ProjectCloseRequested();

  void FloatingPanelCloseRequested();

  void StatusBarDoubleClicked();

#ifdef Q_OS_LINUX
  void ShowNouveauWarning();
#endif

};

OLIVE_NAMESPACE_EXIT

#endif
