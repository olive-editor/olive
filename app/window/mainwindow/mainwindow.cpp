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

#include "mainmenu.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent)
{
#ifdef Q_OS_WINDOWS
  // Qt on Windows has a bug that "de-maximizes" the window when widgets are added, resizing the window beforehand
  // works around that issue and we just set it to whatever size is available
  resize(qApp->desktop()->availableGeometry(this).size());
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

  // Create standard panels
  node_panel_ = PanelManager::instance()->CreatePanel<NodePanel>(this);
  addDockWidget(Qt::TopDockWidgetArea, node_panel_);
  footage_viewer_panel_ = PanelManager::instance()->CreatePanel<FootageViewerPanel>(this);
  addDockWidget(Qt::TopDockWidgetArea, footage_viewer_panel_);
  param_panel_ = PanelManager::instance()->CreatePanel<ParamPanel>(this);
  tabifyDockWidget(footage_viewer_panel_, param_panel_);
  footage_viewer_panel_->raise();
  viewer_panel_ = PanelManager::instance()->CreatePanel<ViewerPanel>(this);
  addDockWidget(Qt::TopDockWidgetArea, viewer_panel_);
  project_panel_ = PanelManager::instance()->CreatePanel<ProjectPanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, project_panel_);
  tool_panel_ = PanelManager::instance()->CreatePanel<ToolPanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, tool_panel_);
  timeline_panel_ = PanelManager::instance()->CreatePanel<TimelinePanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, timeline_panel_);
  audio_monitor_panel_ = PanelManager::instance()->CreatePanel<AudioMonitorPanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, audio_monitor_panel_);
  task_man_panel_ = PanelManager::instance()->CreatePanel<TaskManagerPanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, task_man_panel_);
  curve_panel_ = PanelManager::instance()->CreatePanel<CurvePanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, curve_panel_);

  // FIXME: This is fairly "hardcoded" behavior and doesn't support infinite panels
  connect(node_panel_, &NodePanel::SelectionChanged, param_panel_, &ParamPanel::SetNodes);
  connect(param_panel_, &ParamPanel::SelectedInputChanged, curve_panel_, &CurvePanel::SetInput);
  connect(param_panel_, &ParamPanel::TimebaseChanged, curve_panel_, &CurvePanel::SetTimebase);
  connect(timeline_panel_, &TimelinePanel::TimeChanged, param_panel_, &ParamPanel::SetTime);
  connect(timeline_panel_, &TimelinePanel::TimeChanged, viewer_panel_, &ViewerPanel::SetTime);
  connect(timeline_panel_, &TimelinePanel::TimeChanged, curve_panel_, &CurvePanel::SetTime);
  connect(viewer_panel_, &ViewerPanel::TimeChanged, param_panel_, &ParamPanel::SetTime);
  connect(viewer_panel_, &ViewerPanel::TimeChanged, timeline_panel_, &TimelinePanel::SetTime);
  connect(viewer_panel_, &ViewerPanel::TimeChanged, curve_panel_, &CurvePanel::SetTime);
  connect(param_panel_, &ParamPanel::TimeChanged, viewer_panel_, &ViewerPanel::SetTime);
  connect(param_panel_, &ParamPanel::TimeChanged, timeline_panel_, &TimelinePanel::SetTime);
  connect(param_panel_, &ParamPanel::TimeChanged, curve_panel_, &CurvePanel::SetTime);
  connect(curve_panel_, &CurvePanel::TimeChanged, viewer_panel_, &ViewerPanel::SetTime);
  connect(curve_panel_, &CurvePanel::TimeChanged, timeline_panel_, &TimelinePanel::SetTime);
  connect(curve_panel_, &CurvePanel::TimeChanged, param_panel_, &ParamPanel::SetTime);
}

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

  SetDefaultLayout();
}

// FIXME: Test code
#include <QDir>
#include "common/filefunctions.h"
// End test code

void MainWindow::closeEvent(QCloseEvent *e)
{
  PanelManager::instance()->DeleteAllPanels();

  // FIXME: Test code - We have no cache management and the cache is very much testing only, so we delete it on close
  //                    as to not clog up HDD space
  QDir(GetMediaCacheLocation()).removeRecursively();
  //QDir(GetMediaIndexLocation()).removeRecursively();
  // End test code

  QMainWindow::closeEvent(e);
}

void MainWindow::SetDefaultLayout()
{
  task_man_panel_->setFloating(true);
  task_man_panel_->setVisible(false);
  curve_panel_->setFloating(true);
  curve_panel_->setVisible(false);

  resizeDocks({node_panel_, param_panel_, viewer_panel_},
              {width()/3, width()/3, width()/3},
              Qt::Horizontal);

  resizeDocks({project_panel_, tool_panel_, timeline_panel_, audio_monitor_panel_},
              {width()/4, 1, width(), 1},
              Qt::Horizontal);

  resizeDocks({node_panel_, project_panel_},
              {height()/2, height()/2},
              Qt::Vertical);
}
