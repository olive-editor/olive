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
  // Resizes main window to desktop geometry on startup. Fixes the following issues:
  // * Qt on Windows has a bug that "de-maximizes" the window when widgets are added, resizing the
  //   window beforehand works around that issue and we just set it to whatever size is available.
  // * On Linux, it seems the window starts off at a vastly different size and then maximizes
  //   which throws off the proportions and makes the resulting layout wonky.
  resize(qApp->desktop()->availableGeometry(this).size());

#ifdef Q_OS_WINDOWS
  // Set up taskbar button progress bar (used for some modal tasks like exporting)
  taskbar_btn_id_ = RegisterWindowMessage("TaskbarButtonCreated");
  taskbar_interface_ = nullptr;
#endif

  // Create empty central widget - we don't actually want a central widget (so we set its maximum
  // size to 0,0) but some of Qt's docking/undocking fails without it
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
  connect(status_bar, &MainStatusBar::DoubleClicked, this, &MainWindow::StatusBarDoubleClicked);
  setStatusBar(status_bar);

  // Create standard panels
  node_panel_ = PanelManager::instance()->CreatePanel<NodePanel>(this);
  footage_viewer_panel_ = PanelManager::instance()->CreatePanel<FootageViewerPanel>(this);
  param_panel_ = PanelManager::instance()->CreatePanel<ParamPanel>(this);
  sequence_viewer_panel_ = PanelManager::instance()->CreatePanel<SequenceViewerPanel>(this);
  pixel_sampler_panel_ = PanelManager::instance()->CreatePanel<PixelSamplerPanel>(this);
  AppendProjectPanel();
  tool_panel_ = PanelManager::instance()->CreatePanel<ToolPanel>(this);
  task_man_panel_ = PanelManager::instance()->CreatePanel<TaskManagerPanel>(this);
  AppendTimelinePanel();
  audio_monitor_panel_ = PanelManager::instance()->CreatePanel<AudioMonitorPanel>(this);

  // Make connections to sequence viewer
  connect(node_panel_, &NodePanel::SelectionChanged, param_panel_, &ParamPanel::SetNodes);
  connect(param_panel_, &ParamPanel::RequestSelectNode, node_panel_, &NodePanel::Select);
  connect(sequence_viewer_panel_, &SequenceViewerPanel::TimeChanged, param_panel_, &ParamPanel::SetTimestamp);
  connect(param_panel_, &ParamPanel::TimeChanged, sequence_viewer_panel_, &SequenceViewerPanel::SetTimestamp);
  connect(param_panel_, &ParamPanel::FoundGizmos, sequence_viewer_panel_, &SequenceViewerPanel::SetGizmos);
  connect(PanelManager::instance(), &PanelManager::FocusedPanelChanged, this, &MainWindow::FocusedPanelChanged);

  sequence_viewer_panel_->ConnectTimeBasedPanel(param_panel_);

  footage_viewer_panel_->ConnectPixelSamplerPanel(pixel_sampler_panel_);
  sequence_viewer_panel_->ConnectPixelSamplerPanel(pixel_sampler_panel_);

  UpdateTitle();

  QMetaObject::invokeMethod(this, "SetDefaultLayout", Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
#ifdef Q_OS_WINDOWS
  if (taskbar_interface_) {
    taskbar_interface_->Release();
  }
#endif
}

void MainWindow::LoadLayout(QXmlStreamReader *reader, XMLNodeData &xml_data)
{
  QMetaObject::invokeMethod(this,
                            "LoadLayoutInternal",
                            Qt::BlockingQueuedConnection,
                            Q_ARG(QXmlStreamReader*, reader),
                            Q_ARG(XMLNodeData*, &xml_data));
}

void MainWindow::SaveLayout(QXmlStreamWriter *writer) const
{
  writer->writeStartElement(QStringLiteral("layout"));

  writer->writeStartElement(QStringLiteral("folders"));

  foreach (ProjectPanel* panel, folder_panels_) {
    writer->writeTextElement(QStringLiteral("folder"),
                             QString::number(reinterpret_cast<quintptr>(panel->get_root_index().internalPointer())));
  }

  writer->writeEndElement(); // folders

  writer->writeStartElement(QStringLiteral("timeline"));

  foreach (TimelinePanel* panel, timeline_panels_) {
    writer->writeTextElement(QStringLiteral("sequence"),
                             QString::number(reinterpret_cast<quintptr>(panel->GetConnectedViewer())));
  }

  writer->writeEndElement(); // timeline

  //foreach (ScopePanel* scope, scope_panels_) {
  //  scope->saveGeometry();
  //}

  writer->writeStartElement(QStringLiteral("scopes"));

  foreach (ScopePanel* scope, scope_panels_) {
    writer->writeEmptyElement(QStringLiteral("scope"));
    writer->writeAttribute(QString("geom"), QString(scope->saveGeometry().toBase64()));
    writer->writeAttribute(QString("type"), scope->TypeToName(scope->GetType()));

    //QXmlStreamAttributes attr;
    //attr.append(QStringLiteral("name"), scope->objectName());
    //attr.append(QStringLiteral("type"), scope->TypeToName(scope->GetType()));
    //attr.append(QStringLiteral("floating"), QString::number(scope->isFloating()));
  }

  writer->writeEndElement();  // scopes

  //writer->writeTextElement(QStringLiteral("geometry"), QString(saveGeometry().toBase64()));
  writer->writeTextElement(QStringLiteral("state"), QString(saveState().toBase64()));
  

 

  writer->writeEndElement(); // layout
}

void MainWindow::OpenSequence(Sequence *sequence)
{
  // See if this sequence is already open, and switch to it if so
  foreach (TimelinePanel* tl, timeline_panels_) {
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

  TimelineFocused(sequence->viewer_output());
}

void MainWindow::CloseSequence(Sequence *sequence)
{
  // We defer to RemoveTimelinePanel() to close the panels, which may delete and remove indices from timeline_panels_.
  // We make a copy so that our array here doesn't get ruined by what RemoveTimelinePanel() does
  QList<TimelinePanel*> copy = timeline_panels_;

  foreach (TimelinePanel* tp, copy) {
    if (tp->GetConnectedViewer() == sequence->viewer_output()) {
      RemoveTimelinePanel(tp);
    }
  }
}

bool MainWindow::IsSequenceOpen(Sequence *sequence) const
{
  foreach (TimelinePanel* tp, timeline_panels_) {
    if (tp->GetConnectedViewer() == sequence->viewer_output()) {
      return true;
    }
  }

  return false;
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

void MainWindow::FolderOpen(Project* p, Item *i, bool floating)
{
  ProjectPanel* panel = PanelManager::instance()->CreatePanel<ProjectPanel>(this);

  // Set custom name to distinguish it from regular ProjectPanels
  panel->setObjectName(QStringLiteral("FolderPanel"));

  SetUniquePanelID<ProjectPanel>(panel, folder_panels_);

  panel->set_project(p);
  panel->set_root(i);

  // Tabify with source project panel
  foreach (ProjectPanel* proj_panel, project_panels_) {
    if (proj_panel->project() == p) {
      tabifyDockWidget(proj_panel, panel);
      break;
    }
  }

  panel->setFloating(floating);

  panel->show();
  panel->raise();

  // If the panel is closed, just destroy it
  panel->SetSignalInsteadOfClose(true);
  panel->setProperty("parent_list", reinterpret_cast<quintptr>(&folder_panels_));
  connect(panel, &ProjectPanel::CloseRequested, this, &MainWindow::FloatingPanelCloseRequested);

  folder_panels_.append(panel);
}

ScopePanel *MainWindow::AppendScopePanel()
{
  return AppendFloatingPanelInternal<ScopePanel>(scope_panels_);
}

CurvePanel *MainWindow::AppendCurvePanel()
{
  CurvePanel* p = AppendFloatingPanelInternal<CurvePanel>(curve_panels_);

  sequence_viewer_panel_->ConnectTimeBasedPanel(p);

  return p;

  /*connect(panel, &TimelinePanel::TimeChanged, curve_panel_, &CurvePanel::SetTime);
  connect(curve_panel_, &CurvePanel::TimeChanged, panel, &TimelinePanel::SetTime);
  connect(param_panel_, &ParamPanel::SelectedInputChanged, curve_panel_, &CurvePanel::SetInput);
  connect(param_panel_, &ParamPanel::TimebaseChanged, curve_panel_, &CurvePanel::SetTimebase);
  connect(param_panel_, &ParamPanel::TimeTargetChanged, curve_panel_, &CurvePanel::SetTimeTarget);
  connect(param_panel_, &ParamPanel::TimeChanged, curve_panel_, &CurvePanel::SetTime);
  connect(curve_panel_, &CurvePanel::TimeChanged, sequence_viewer_panel_, &SequenceViewerPanel::SetTime);
  connect(curve_panel_, &CurvePanel::TimeChanged, param_panel_, &ParamPanel::SetTime);
  sequence_viewer_panel_->ConnectTimeBasedPanel(curve_panel_);
  connect(sequence_viewer_panel_, &SequenceViewerPanel::TimeChanged, curve_panel_, &CurvePanel::SetTime);*/
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

void MainWindow::ProjectOpen(Project *p)
{
  // See if this project is already open, and switch to it if so
  foreach (ProjectPanel* pl, project_panels_) {
    if (pl->project() == p) {
      pl->raise();
      return;
    }
  }

  ProjectPanel* panel;

  if (!project_panels_.first()->project()) {
    panel = project_panels_.first();
  } else {
    panel = AppendProjectPanel();
  }

  panel->set_project(p);
}

void MainWindow::ProjectClose(Project *p)
{
  // Close project from project panel
  foreach (ProjectPanel* panel, project_panels_) {
    if (panel->project() == p) {
      RemoveProjectPanel(panel);
    }
  }

  foreach (ProjectPanel* panel, folder_panels_) {
    if (panel->project() == p) {
      panel->close();
    }
  }

  // Close any open sequences from project
  QList<ItemPtr> open_sequences = p->get_items_of_type(Item::kSequence);

  foreach (ItemPtr item, open_sequences) {
    Sequence* seq = static_cast<Sequence*>(item.get());

    if (IsSequenceOpen(seq)) {
      CloseSequence(seq);
    }
  }
}

void MainWindow::closeEvent(QCloseEvent *e)
{
  // Try to close all projects (this will return false if the user chooses not to close)
  if (!Core::instance()->CloseAllProjects(false)) {
    e->ignore();
    return;
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

void MainWindow::StatusBarDoubleClicked()
{
  task_man_panel_->show();
  task_man_panel_->raise();
}

void MainWindow::UpdateTitle()
{
  if (Core::instance()->GetActiveProject()) {
    setWindowTitle(QStringLiteral("%1 %2 - [*]%3").arg(QApplication::applicationName(),
                                                       QApplication::applicationVersion(),
                                                       Core::instance()->GetActiveProject()->pretty_filename()));
  } else {
    setWindowTitle(QStringLiteral("%1 %2").arg(QApplication::applicationName(),
                                               QApplication::applicationVersion()));
  }
}

void MainWindow::TimelineCloseRequested()
{
  RemoveTimelinePanel(static_cast<TimelinePanel*>(sender()));
}

void MainWindow::ProjectCloseRequested()
{
  ProjectPanel* panel = static_cast<ProjectPanel*>(sender());
  Project* p = panel->project();

  Core::instance()->CloseProject(Core::instance()->GetSharedPtrFromProject(p), true);
}

void MainWindow::FloatingPanelCloseRequested()
{
  PanelWidget* panel = static_cast<PanelWidget*>(sender());

  quintptr list_ptr = panel->property("parent_list").value<quintptr>();
  QList<PanelWidget*>* list = reinterpret_cast< QList<PanelWidget*>* >(list_ptr);

  list->removeOne(panel);
  panel->deleteLater();
}

void MainWindow::LoadLayoutInternal(QXmlStreamReader *reader, XMLNodeData *xml_data)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("folders")) {

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("folder")) {
          quintptr item_id = reader->readElementText().toULongLong();

          Item* open_item = xml_data->item_ptrs.value(item_id);

          if (open_item) {
            FolderOpen(open_item->project(), open_item, true);
          }
        } else {
          reader->skipCurrentElement();
        }
      }

    } else if (reader->name() == QStringLiteral("timeline")) {

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("sequence")) {
          quintptr item_id = reader->readElementText().toULongLong();

          Sequence* open_seq = dynamic_cast<Sequence*>(xml_data->item_ptrs.value(item_id));

          if (open_seq) {
            OpenSequence(open_seq);
          }
        } else {
          reader->skipCurrentElement();
        }
      }

    } else if (reader->name() == QStringLiteral("state")) {

      QByteArray state = QByteArray::fromBase64(reader->readElementText().toLatin1());

      restoreState(state);

    } else if (reader->name() == QStringLiteral("geometry")) {
      QByteArray state = QByteArray::fromBase64(reader->readElementText().toLatin1());

      //restoreGeometry(state);

    } else if (reader->name() == QStringLiteral("scopes")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("scope")) {
          XMLAttributeLoop(reader, attr) {
            printf("attrib: %s, %s\n", attr.name().toString().toStdString().c_str(),
                   attr.value().toString().toStdString().c_str());
          }
          QXmlStreamAttributes attr = reader->attributes();
          sequence_viewer_panel_->LoadScopePanel(attr);
          reader->readNext(); // hack to read next elemnts
        }
       }
    } else {
      reader->skipCurrentElement();
    }
  }
}

TimelinePanel* MainWindow::AppendTimelinePanel()
{
  TimelinePanel* panel = AppendPanelInternal<TimelinePanel>(timeline_panels_);

  connect(panel, &PanelWidget::CloseRequested, this, &MainWindow::TimelineCloseRequested);
  connect(panel, &TimelinePanel::TimeChanged, param_panel_, &ParamPanel::SetTimestamp);
  connect(panel, &TimelinePanel::TimeChanged, sequence_viewer_panel_, &SequenceViewerPanel::SetTimestamp);
  connect(panel, &TimelinePanel::SelectionChanged, node_panel_, &NodePanel::SelectBlocks);
  connect(param_panel_, &ParamPanel::TimeChanged, panel, &TimelinePanel::SetTimestamp);
  connect(sequence_viewer_panel_, &SequenceViewerPanel::TimeChanged, panel, &TimelinePanel::SetTimestamp);
  connect(sequence_viewer_panel_->video_renderer(), &VideoRenderBackend::CachedTimeReady, panel->ruler(), &TimeRuler::CacheTimeReady);
  connect(sequence_viewer_panel_->video_renderer(), &VideoRenderBackend::RangeInvalidated, panel->ruler(), &TimeRuler::CacheInvalidatedRange);

  sequence_viewer_panel_->ConnectTimeBasedPanel(panel);

  return panel;
}

ProjectPanel *MainWindow::AppendProjectPanel()
{
  ProjectPanel* panel = AppendPanelInternal<ProjectPanel>(project_panels_);

  connect(panel, &PanelWidget::CloseRequested, this, &MainWindow::ProjectCloseRequested);
  connect(panel, &ProjectPanel::ProjectNameChanged, this, &MainWindow::UpdateTitle);

  return panel;
}

void MainWindow::RemoveTimelinePanel(TimelinePanel *panel)
{
  // Stop showing this timeline in the viewer
  TimelineFocused(nullptr);

  if (timeline_panels_.size() == 1) {
    // Leave our single remaining timeline panel open
    panel->ConnectViewerNode(nullptr);
  } else {
    timeline_panels_.removeOne(panel);
    panel->deleteLater();
  }
}

void MainWindow::RemoveProjectPanel(ProjectPanel *panel)
{
  if (project_panels_.size() == 1) {
    panel->set_project(nullptr);
  } else {
    project_panels_.removeOne(panel);
    panel->deleteLater();
  }
}

void MainWindow::TimelineFocused(ViewerOutput* viewer)
{
  sequence_viewer_panel_->ConnectViewerNode(viewer);

  Sequence* seq = nullptr;

  if (viewer) {
    seq = static_cast<Sequence*>(viewer->parent());
  }

  node_panel_->SetGraph(seq);
}

void MainWindow::FocusedPanelChanged(PanelWidget *panel)
{
  TimelinePanel* timeline = dynamic_cast<TimelinePanel*>(panel);

  if (timeline) {
    TimelineFocused(timeline->GetConnectedViewer());
    return;
  }

  ProjectPanel* project = dynamic_cast<ProjectPanel*>(panel);

  if (project) {
    UpdateTitle();
    return;
  }
}

void MainWindow::SetDefaultLayout()
{
  node_panel_->show();
  addDockWidget(Qt::TopDockWidgetArea, node_panel_);

  footage_viewer_panel_->show();
  addDockWidget(Qt::TopDockWidgetArea, footage_viewer_panel_);

  param_panel_->show();
  tabifyDockWidget(footage_viewer_panel_, param_panel_);
  footage_viewer_panel_->raise();

  sequence_viewer_panel_->show();
  addDockWidget(Qt::TopDockWidgetArea, sequence_viewer_panel_);

  pixel_sampler_panel_->hide();
  pixel_sampler_panel_->setFloating(true);
  addDockWidget(Qt::TopDockWidgetArea, pixel_sampler_panel_);

  project_panels_.first()->show();
  addDockWidget(Qt::BottomDockWidgetArea, project_panels_.first());

  tool_panel_->show();
  addDockWidget(Qt::BottomDockWidgetArea, tool_panel_);

  timeline_panels_.first()->show();
  addDockWidget(Qt::BottomDockWidgetArea, timeline_panels_.first());

  task_man_panel_->hide();
  task_man_panel_->setFloating(true);
  addDockWidget(Qt::BottomDockWidgetArea, task_man_panel_);

  audio_monitor_panel_->show();
  addDockWidget(Qt::BottomDockWidgetArea, audio_monitor_panel_);

  resizeDocks({node_panel_, param_panel_, sequence_viewer_panel_},
  {width()/3, width()/3, width()/3},
              Qt::Horizontal);

  resizeDocks({project_panels_.first(), tool_panel_, timeline_panels_.first(), audio_monitor_panel_},
  {width()/4, 1, width(), 1},
              Qt::Horizontal);

  resizeDocks({node_panel_, project_panels_.first()},
  {height()/2, height()/2},
              Qt::Vertical);
}

template<typename T>
T *MainWindow::AppendPanelInternal(QList<T*>& list)
{
  T* panel = PanelManager::instance()->CreatePanel<T>(this);

  SetUniquePanelID(panel, list);

  if (!list.isEmpty()) {
    tabifyDockWidget(list.last(), panel);
  }

  // For some reason raise() on its own doesn't do anything, we need both
  panel->show();
  panel->raise();

  list.append(panel);

  // Let us handle the panel closing rather than the panel itself
  panel->SetSignalInsteadOfClose(true);

  return panel;
}

template<typename T>
void MainWindow::SetUniquePanelID(T *panel, const QList<T *> &list)
{
  // Set unique object name so it can be identified by QMainWindow's save and restore state functions
  panel->setObjectName(panel->objectName().append(QString::number(list.size())));
}

template<typename T>
T *MainWindow::AppendFloatingPanelInternal(QList<T *> &list)
{
  T* panel = PanelManager::instance()->CreatePanel<T>(this);

  SetUniquePanelID(panel, list);

  panel->setFloating(true);
  panel->show();

  // add new panel to panel_list_
  list.append(panel);

  panel->SetSignalInsteadOfClose(true);
  connect(panel, &PanelWidget::CloseRequested, this, &MainWindow::FloatingPanelCloseRequested);

  panel->setProperty("parent_list", reinterpret_cast<quintptr>(&list));

  return panel;
}

OLIVE_NAMESPACE_EXIT
