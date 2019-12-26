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

// Panel objects
#include "panel/panelmanager.h"
#include "panel/audiomonitor/audiomonitor.h"
#include "panel/node/node.h"
#include "panel/param/param.h"
#include "panel/project/project.h"
#include "panel/taskmanager/taskmanager.h"
#include "panel/timeline/timeline.h"
#include "panel/tool/tool.h"
#include "panel/viewer/viewer.h"

// Main menu bar
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
  NodePanel* node_panel = PanelManager::instance()->CreatePanel<NodePanel>(this);
  addDockWidget(Qt::TopDockWidgetArea, node_panel);

  ParamPanel* param_panel = PanelManager::instance()->CreatePanel<ParamPanel>(this);
  addDockWidget(Qt::TopDockWidgetArea, param_panel);

  ViewerPanel* viewer_panel2 = PanelManager::instance()->CreatePanel<ViewerPanel>(this);
  addDockWidget(Qt::TopDockWidgetArea, viewer_panel2);

  ProjectPanel* project_panel = PanelManager::instance()->CreatePanel<ProjectPanel>(this);
  project_panel->set_project(p);
  addDockWidget(Qt::BottomDockWidgetArea, project_panel);

  ToolPanel* tool_panel = PanelManager::instance()->CreatePanel<ToolPanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, tool_panel);

  TimelinePanel* timeline_panel = PanelManager::instance()->CreatePanel<TimelinePanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, timeline_panel);

  AudioMonitorPanel* audio_monitor_panel = PanelManager::instance()->CreatePanel<AudioMonitorPanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, audio_monitor_panel);

  TaskManagerPanel* task_man_panel = PanelManager::instance()->CreatePanel<TaskManagerPanel>(this);
  addDockWidget(Qt::BottomDockWidgetArea, task_man_panel);
  task_man_panel->setFloating(true);
  task_man_panel->setVisible(false);

  connect(node_panel, SIGNAL(SelectionChanged(QList<Node*>)), param_panel, SLOT(SetNodes(QList<Node*>)));
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
