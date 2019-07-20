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

#include <QDebug>

// Panel objects
#include "panel/node/node.h"
#include "panel/param/param.h"
#include "panel/project/project.h"
#include "panel/timeline/timeline.h"
#include "panel/tool/tool.h"
#include "panel/viewer/viewer.h"

// Main menu bar
#include "mainmenu.h"

// FIXME: Test code
#include "node/input/media/media.h"
#include "node/output/viewer/viewer.h"
#include "node/generator/solid/solid.h"
// End test code

olive::MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent)
{
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

void olive::MainWindow::ProjectOpen(Project* p)
{
  // TODO Use settings data to create panels and restore state if they exist
  ProjectPanel* project_panel = new ProjectPanel(this);
  project_panel->set_project(p);
  addDockWidget(Qt::TopDockWidgetArea, project_panel);

  ViewerPanel* viewer_panel1 = new ViewerPanel(this);
  addDockWidget(Qt::TopDockWidgetArea, viewer_panel1);

  ViewerPanel* viewer_panel2 = new ViewerPanel(this);
  addDockWidget(Qt::TopDockWidgetArea, viewer_panel2);

  ToolPanel* tool_panel = new ToolPanel(this);
  addDockWidget(Qt::BottomDockWidgetArea, tool_panel);

  NodePanel* node_panel = new NodePanel(this);
  addDockWidget(Qt::BottomDockWidgetArea, node_panel);

  ParamPanel* param_panel = new ParamPanel(this);
  addDockWidget(Qt::BottomDockWidgetArea, param_panel);

  TimelinePanel* timeline_panel = new TimelinePanel(this);
  addDockWidget(Qt::BottomDockWidgetArea, timeline_panel);

  connect(node_panel, SIGNAL(SelectionChanged(QList<Node*>)), param_panel, SLOT(SetNodes(QList<Node*>)));

  // FIXME: Test code
  NodeGraph* graph = new NodeGraph();
  graph->setParent(this);
  graph->set_name("New Graph");
  ViewerOutput* vo = new ViewerOutput();
  vo->AttachViewer(viewer_panel2);
  graph->AddNode(vo);
  SolidGenerator* sg = new SolidGenerator();
  NodeInput::ConnectEdge(sg->texture_output(), vo->texture_input());
  graph->AddNode(sg);
  MediaInput* ii = new MediaInput();
  graph->AddNode(ii);
  node_panel->SetGraph(graph);
  // End test code
}
