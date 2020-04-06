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

#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QMessageBox>

#include "mainmenu.h"
#include "mainstatusbar.h"

OLIVE_NAMESPACE_ENTER

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent)
{
#ifdef Q_OS_WINDOWS
  // Qt on Windows has a bug that "de-maximizes" the window when widgets are added, resizing the window beforehand
  // works around that issue and we just set it to whatever size is available
  resize(qApp->desktop()->availableGeometry(this).size());

  // Set up taskbar button progress bar (used for some modal tasks like exporting)
  taskbar_btn_id_ = RegisterWindowMessage("TaskbarButtonCreated");
  taskbar_interface_ = nullptr;
#endif

  // Create empty central widget - we don't actually want a central widget but some of Qt's docking/undocking fails
  // without it
  QWidget* centralWidget = new QWidget(this);
  centralWidget->setMaximumSize(QSize(0, 0));
  setCentralWidget(centralWidget);

  // Set tabs to be on top of panels (default behavior is bottom)
  setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

  // Allow panels to be tabbed within each other
  setDockNestingEnabled(true);

  // Create and set main menu
  MainMenu* main_menu = new MainMenu(this);
  setMenuBar(main_menu);

  // Create and set status bar
  MainStatusBar* status_bar = new MainStatusBar(this);
  status_bar->ConnectTaskManager(TaskManager::instance());
  setStatusBar(status_bar);

  // Create standard panels
  node_panel_ = PanelManager::instance()->CreatePanel<NodePanel>(this);
  addDockWidget(Qt::TopDockWidgetArea, node_panel_);
  footage_viewer_panel_ = PanelManager::instance()->CreatePanel<FootageViewerPanel>(this);
  addDockWidget(Qt::TopDockWidgetArea, footage_viewer_panel_);
  param_panel_ = PanelManager::instance()->CreatePanel<ParamPanel>(this);
  tabifyDockWidget(footage_viewer_panel_, param_panel_);
  footage_viewer_panel_->raise();
  sequence_viewer_panel_ = PanelManager::instance()->CreatePanel<SequenceViewerPanel>(this);
  addDockWidget(Qt::TopDockWidgetArea, sequence_viewer_panel_);
  pixel_sampler_panel_ = PanelManager::instance()->CreatePanel<PixelSamplerPanel>(this);
  addDockWidget(Qt::TopDockWidgetArea, pixel_sampler_panel_);
  project_panel_ = PanelManager::instance()->CreatePanel<ProjectPanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, project_panel_);
  tool_panel_ = PanelManager::instance()->CreatePanel<ToolPanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, tool_panel_);
  task_man_panel_ = PanelManager::instance()->CreatePanel<TaskManagerPanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, task_man_panel_);
  curve_panel_ = PanelManager::instance()->CreatePanel<CurvePanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, curve_panel_);
  AppendTimelinePanel();
  audio_monitor_panel_ = PanelManager::instance()->CreatePanel<AudioMonitorPanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, audio_monitor_panel_);

  // Make connections to sequence viewer
  connect(node_panel_, &NodePanel::SelectionChanged, param_panel_, &ParamPanel::SetNodes);
  connect(param_panel_, &ParamPanel::SelectedInputChanged, curve_panel_, &CurvePanel::SetInput);
  connect(param_panel_, &ParamPanel::TimebaseChanged, curve_panel_, &CurvePanel::SetTimebase);
  connect(param_panel_, &ParamPanel::TimeTargetChanged, curve_panel_, &CurvePanel::SetTimeTarget);
  connect(sequence_viewer_panel_, &SequenceViewerPanel::TimeChanged, param_panel_, &ParamPanel::SetTime);
  connect(sequence_viewer_panel_, &SequenceViewerPanel::TimeChanged, curve_panel_, &CurvePanel::SetTime);
  connect(param_panel_, &ParamPanel::TimeChanged, sequence_viewer_panel_, &SequenceViewerPanel::SetTime);
  connect(param_panel_, &ParamPanel::TimeChanged, curve_panel_, &CurvePanel::SetTime);
  connect(curve_panel_, &CurvePanel::TimeChanged, sequence_viewer_panel_, &SequenceViewerPanel::SetTime);
  connect(curve_panel_, &CurvePanel::TimeChanged, param_panel_, &ParamPanel::SetTime);

  connect(PanelManager::instance(), &PanelManager::FocusedPanelChanged, this, &MainWindow::FocusedPanelChanged);

  sequence_viewer_panel_->ConnectTimeBasedPanel(param_panel_);
  sequence_viewer_panel_->ConnectTimeBasedPanel(curve_panel_);

  footage_viewer_panel_->ConnectPixelSamplerPanel(pixel_sampler_panel_);
  sequence_viewer_panel_->ConnectPixelSamplerPanel(pixel_sampler_panel_);

  connect(project_panel_, &ProjectPanel::ProjectNameChanged, this, &MainWindow::UpdateTitle);
  UpdateTitle();
}

MainWindow::~MainWindow()
{
#ifdef Q_OS_WINDOWS
  if (taskbar_interface_) {
    taskbar_interface_->Release();
  }
#endif
}

void MainWindow::OpenSequence(Sequence *sequence)
{
  // See if this sequence is already open, and switch to it if so
  for (int i=0;i<timeline_panels_.size();i++) {
    TimelinePanel* tl = timeline_panels_.at(i);

    if (tl->GetConnectedViewer() == sequence->viewer_output()) {
      tl->raise();
      return;
    }
  }

  // See if we have any sequences open or not
  TimelinePanel* panel;

  if (!timeline_panels_.first()->GetConnectedViewer()) {
    panel = timeline_panels_.first();
  } else {
    panel = AppendTimelinePanel();
  }

  panel->ConnectViewerNode(sequence->viewer_output());

  TimelineFocused(panel);
}

void MainWindow::CloseSequence(Sequence *sequence)
{
  for (int i=0;i<timeline_panels_.size();i++) {
    TimelinePanel* tl = timeline_panels_.at(i);

    if (tl->GetConnectedViewer() == sequence->viewer_output()) {
      tl->ConnectViewerNode(nullptr);

      if (timeline_panels_.size() > 1) {
        delete tl;
        timeline_panels_.removeAt(i);
        i--;
      }
    }
  }
}

#ifdef Q_OS_WINDOWS
void MainWindow::SetTaskbarButtonState(TBPFLAG flags)
{
  if (taskbar_interface_) {
    taskbar_interface_->SetProgressState(reinterpret_cast<HWND>(this->winId()), flags);
  }
}

void MainWindow::SetTaskbarButtonProgress(int value, int max)
{
  if (taskbar_interface_) {
    taskbar_interface_->SetProgressValue(reinterpret_cast<HWND>(this->winId()), value, max);
  }
}
#endif

void MainWindow::SetFullscreen(bool fullscreen)
{
  if (fullscreen) {
    setWindowState(windowState() | Qt::WindowFullScreen);
  } else {
    setWindowState(windowState() & ~Qt::WindowFullScreen);
  }
}

void MainWindow::ToggleMaximizedPanel()
{
  if (premaximized_state_.isEmpty()) {
    // Assume nothing is maximized at the moment

    // Find the currently focused panel
    PanelWidget* currently_hovered = PanelManager::instance()->CurrentlyHovered();

    // If no panel is hovered, do nothing
    if (currently_hovered == nullptr) {
      return;
    }

    // If this panel is not actually on the main window, this is a no-op
    if (currently_hovered->isFloating()) {
      return;
    }

    // Save the current state so it can be restored later
    premaximized_state_ = saveState();

    // For every other panel that is on the main window, hide it
    foreach (PanelWidget* panel, PanelManager::instance()->panels()) {
      if (!panel->isFloating() && panel != currently_hovered) {
        panel->setVisible(false);
      }
    }
  } else {
    // Assume we are currently maximized, restore the state
    restoreState(premaximized_state_);
    premaximized_state_.clear();
  }
}

void MainWindow::ProjectOpen(Project* p)
{
  // FIXME Use settings data to create panels and restore state if they exist
  project_panel_->set_project(p);

  UpdateTitle();

  SetDefaultLayout();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
  if (isWindowModified()) {
    QMessageBox mb(this);

    mb.setIcon(QMessageBox::Question);
    mb.setWindowTitle(tr("Unsaved Changes"));
    mb.setText(tr("The project '%1' has unsaved changes. Would you like to save them?")
               .arg(Core::instance()->GetActiveProject()->name()));

    QPushButton* yes_btn = mb.addButton(tr("Save"), QMessageBox::YesRole);
    mb.addButton(tr("Don't Save"), QMessageBox::NoRole);
    QPushButton* cancel_btn = mb.addButton(QMessageBox::Cancel);

    mb.exec();

    if (mb.clickedButton() == cancel_btn
        || (mb.clickedButton() == yes_btn && !Core::instance()->SaveActiveProject())) {
      // Don't close if the user clicked cancel on this messagebox OR they cancelled a save as operation
      e->ignore();
      return;
    }
  }

  // Close viewers first since we don't want to delete any nodes while they might be mid-render
  QList<ViewerPanelBase*> viewers = PanelManager::instance()->GetPanelsOfType<ViewerPanelBase>();
  foreach (ViewerPanelBase* viewer, viewers) {
    viewer->ConnectViewerNode(nullptr);
  }

  PanelManager::instance()->DeleteAllPanels();

  QMainWindow::closeEvent(e);
}

#ifdef Q_OS_WINDOWS
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
  if (static_cast<MSG*>(message)->message == taskbar_btn_id_) {
    // Attempt to create taskbar button progress handle
    HRESULT hr = CoCreateInstance(CLSID_TaskbarList,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_ITaskbarList3,
                                  reinterpret_cast<void**>(&taskbar_interface_));

    if (SUCCEEDED(hr)) {
      hr = taskbar_interface_->HrInit();

      if (FAILED(hr)) {
        taskbar_interface_->Release();
        taskbar_interface_ = nullptr;
      }
    }
  }

  return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

void MainWindow::UpdateTitle()
{
  if (project_panel_->project()) {
    setWindowTitle(QStringLiteral("%1 %2 - [*]%3").arg(QApplication::applicationName(),
                                                       QApplication::applicationVersion(),
                                                       project_panel_->project()->pretty_filename()));
  } else {
    setWindowTitle(QStringLiteral("%1 %2").arg(QApplication::applicationName(),
                                               QApplication::applicationVersion()));
  }
}

TimelinePanel* MainWindow::AppendTimelinePanel()
{
  TimelinePanel* panel = PanelManager::instance()->CreatePanel<TimelinePanel>(this);;

  if (timeline_panels_.isEmpty()) {
    addDockWidget(Qt::BottomDockWidgetArea, panel);
  } else {
    tabifyDockWidget(timeline_panels_.last(), panel);

    // For some reason raise() on its own doesn't do anything, we need both
    panel->show();
    panel->raise();
  }

  timeline_panels_.append(panel);

  connect(panel, &TimelinePanel::TimeChanged, param_panel_, &ParamPanel::SetTime);
  connect(panel, &TimelinePanel::TimeChanged, sequence_viewer_panel_, &SequenceViewerPanel::SetTime);
  connect(panel, &TimelinePanel::TimeChanged, curve_panel_, &CurvePanel::SetTime);
  connect(panel, &TimelinePanel::SelectionChanged, node_panel_, &NodePanel::SelectWithDependencies);
  connect(param_panel_, &ParamPanel::TimeChanged, panel, &TimelinePanel::SetTime);
  connect(curve_panel_, &CurvePanel::TimeChanged, panel, &TimelinePanel::SetTime);
  connect(sequence_viewer_panel_, &SequenceViewerPanel::TimeChanged, panel, &TimelinePanel::SetTime);
  connect(sequence_viewer_panel_->video_renderer(), &VideoRenderBackend::CachedTimeReady, panel->ruler(), &TimeRuler::CacheTimeReady);
  connect(sequence_viewer_panel_->video_renderer(), &VideoRenderBackend::RangeInvalidated, panel->ruler(), &TimeRuler::CacheInvalidatedRange);

  sequence_viewer_panel_->ConnectTimeBasedPanel(panel);

  return panel;
}

void MainWindow::TimelineFocused(TimelinePanel* panel)
{
  sequence_viewer_panel_->ConnectViewerNode(panel->GetConnectedViewer());

  Sequence* seq = nullptr;

  if (panel->GetConnectedViewer()) {
    seq = static_cast<Sequence*>(panel->GetConnectedViewer()->parent());
  }

  node_panel_->SetGraph(seq);
}

void MainWindow::FocusedPanelChanged(PanelWidget *panel)
{
  TimelinePanel* timeline = dynamic_cast<TimelinePanel*>(panel);

  if (timeline) {
    TimelineFocused(timeline);
  }
}

void MainWindow::SetDefaultLayout()
{
  task_man_panel_->close();
  task_man_panel_->setFloating(true);
  curve_panel_->close();
  curve_panel_->setFloating(true);
  pixel_sampler_panel_->close();
  pixel_sampler_panel_->setFloating(true);

  resizeDocks({node_panel_, param_panel_, sequence_viewer_panel_},
  {width()/3, width()/3, width()/3},
              Qt::Horizontal);

  resizeDocks({project_panel_, tool_panel_, timeline_panels_.first(), audio_monitor_panel_},
  {width()/4, 1, width(), 1},
              Qt::Horizontal);

  resizeDocks({node_panel_, project_panel_},
  {height()/2, height()/2},
              Qt::Vertical);
}

OLIVE_NAMESPACE_EXIT
