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

#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QMessageBox>

#ifdef Q_OS_LINUX
#include <QOffscreenSurface>
#endif

#include "dialog/about/about.h"
#include "mainmenu.h"
#include "mainstatusbar.h"

namespace olive {

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
  taskbar_btn_id_ = RegisterWindowMessage(TEXT("TaskbarButtonCreated"));
  taskbar_interface_ = nullptr;
#endif

  first_show_ = true;

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

  LoadCustomShortcuts();

  // Create and set status bar
  MainStatusBar* status_bar = new MainStatusBar(this);
  status_bar->ConnectTaskManager(TaskManager::instance());
  connect(status_bar, &MainStatusBar::DoubleClicked, this, &MainWindow::StatusBarDoubleClicked);
  setStatusBar(status_bar);

  // Create standard panels
  node_panel_ = new NodePanel(this);
  footage_viewer_panel_ = new FootageViewerPanel(this);
  param_panel_ = new ParamPanel(this);
  curve_panel_ = new CurvePanel(this);
  sequence_viewer_panel_ = new SequenceViewerPanel(this);
  pixel_sampler_panel_ = new PixelSamplerPanel(this);
  AppendProjectPanel();
  tool_panel_ = new ToolPanel(this);
  task_man_panel_ = new TaskManagerPanel(this);
  AppendTimelinePanel();
  audio_monitor_panel_ = new AudioMonitorPanel(this);
  scope_panel_ = new ScopePanel(this);

  // Make node-related connections
  connect(node_panel_, &NodePanel::NodesSelected, param_panel_, &ParamPanel::SelectNodes);
  connect(node_panel_, &NodePanel::NodesDeselected, param_panel_, &ParamPanel::DeselectNodes);
  connect(node_panel_, &NodePanel::NodeGroupOpenRequested, this, &MainWindow::NodeGroupRequested);
  connect(param_panel_, &ParamPanel::RequestSelectNode, this, [this](const QVector<Node*>& target){
    node_panel_->Select(target, true);
  });
  connect(param_panel_, &ParamPanel::FocusedNodeChanged, sequence_viewer_panel_, &ViewerPanel::SetGizmos);
  connect(param_panel_, &ParamPanel::FocusedNodeChanged, curve_panel_, &CurvePanel::SetNode);

  // Connect time signals together
  connect(sequence_viewer_panel_, &SequenceViewerPanel::TimeChanged, param_panel_, &ParamPanel::SetTime);
  connect(sequence_viewer_panel_, &SequenceViewerPanel::TimeChanged, curve_panel_, &NodeTablePanel::SetTime);
  connect(param_panel_, &ParamPanel::TimeChanged, sequence_viewer_panel_, &SequenceViewerPanel::SetTime);
  connect(param_panel_, &ParamPanel::TimeChanged, curve_panel_, &NodeTablePanel::SetTime);
  connect(curve_panel_, &ParamPanel::TimeChanged, sequence_viewer_panel_, &SequenceViewerPanel::SetTime);
  connect(curve_panel_, &ParamPanel::TimeChanged, param_panel_, &NodeTablePanel::SetTime);

  connect(PanelManager::instance(), &PanelManager::FocusedPanelChanged, this, &MainWindow::FocusedPanelChanged);

  sequence_viewer_panel_->ConnectTimeBasedPanel(param_panel_);
  sequence_viewer_panel_->ConnectTimeBasedPanel(curve_panel_);

  scope_panel_->SetViewerPanel(sequence_viewer_panel_);

  UpdateTitle();

  QMetaObject::invokeMethod(this, &MainWindow::SetDefaultLayout, Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
#ifdef Q_OS_WINDOWS
  if (taskbar_interface_) {
    taskbar_interface_->Release();
  }
#endif
}

void MainWindow::LoadLayout(const MainWindowLayoutInfo &info)
{
  foreach (Folder* folder, info.open_folders()) {
    FolderOpen(folder->project(), folder, true);
  }

  foreach (const MainWindowLayoutInfo::OpenSequence& sequence, info.open_sequences()) {
    TimelinePanel* panel = OpenSequence(sequence.sequence, info.open_sequences().size() == 1);
    panel->RestoreSplitterState(sequence.panel_state);
  }

  restoreState(info.state());
}

MainWindowLayoutInfo MainWindow::SaveLayout() const
{
  MainWindowLayoutInfo info;

  foreach (ProjectPanel* panel, folder_panels_) {
    if (panel->project()) {
      info.add_folder(panel->get_root());
    }
  }

  foreach (TimelinePanel* panel, timeline_panels_) {
    if (panel->GetConnectedViewer()) {
      info.add_sequence({static_cast<Sequence*>(panel->GetConnectedViewer()),
                         panel->SaveSplitterState()});
    }
  }

  info.set_state(saveState());

  return info;
}

TimelinePanel* MainWindow::OpenSequence(Sequence *sequence, bool enable_focus)
{
  // See if this sequence is already open, and switch to it if so
  foreach (TimelinePanel* tl, timeline_panels_) {
    if (tl->GetConnectedViewer() == sequence) {
      tl->raise();
      return tl;
    }
  }

  // See if we have any sequences open or not
  TimelinePanel* panel;

  if (!timeline_panels_.first()->GetConnectedViewer()) {
    panel = timeline_panels_.first();
  } else {
    panel = AppendTimelinePanel();
    enable_focus = false;
  }

  panel->ConnectViewerNode(sequence);

  if (enable_focus) {
    TimelineFocused(sequence);
    UpdateAudioMonitorParams(sequence);
  }

  return panel;
}

void MainWindow::CloseSequence(Sequence *sequence)
{
  // We defer to RemoveTimelinePanel() to close the panels, which may delete and remove indices from timeline_panels_.
  // We make a copy so that our array here doesn't get ruined by what RemoveTimelinePanel() does
  QList<TimelinePanel*> copy = timeline_panels_;

  foreach (TimelinePanel* tp, copy) {
    if (tp->GetConnectedViewer() == sequence) {
      RemoveTimelinePanel(tp);
    }
  }
}

bool MainWindow::IsSequenceOpen(Sequence *sequence) const
{
  foreach (TimelinePanel* tp, timeline_panels_) {
    if (tp->GetConnectedViewer() == sequence) {
      return true;
    }
  }

  return false;
}

void MainWindow::FolderOpen(Project* p, Folder *i, bool floating)
{
  ProjectPanel* panel = new ProjectPanel(this);

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

void MainWindow::OpenNodeInViewer(ViewerOutput *node)
{
  if (viewer_panels_.contains(node)) {
    // This node already has a viewer, raise it
    viewer_panels_.value(node)->raise();
  } else {
    // Create a viewer for this node
    ViewerPanel* viewer = new ViewerPanel(this);

    viewer->SetSignalInsteadOfClose(true);
    viewer->setFloating(true);
    viewer->setVisible(true);
    viewer->ConnectViewerNode(node);

    connect(viewer, &ViewerPanel::CloseRequested, this, &MainWindow::ViewerCloseRequested);
    connect(node, &ViewerOutput::RemovedFromGraph, this, &MainWindow::ViewerWithPanelRemovedFromGraph);

    viewer_panels_.insert(node, viewer);
  }
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

    // If no panel is hovered, fallback to the currently active panel
    if (!currently_hovered) {
      currently_hovered = PanelManager::instance()->CurrentlyFocused();

      // If no panel is hovered or focused, do nothing
      if (!currently_hovered) {
        return;
      }
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
  panel->setFocus();
}

void MainWindow::ProjectClose(Project *p)
{
  // Close any nodes open in TimeBasedWidgets
  foreach (PanelWidget* panel, PanelManager::instance()->panels()) {
    TimeBasedPanel* tbp = dynamic_cast<TimeBasedPanel*>(panel);

    if (tbp && tbp->GetConnectedViewer() && tbp->GetConnectedViewer()->project() == p) {
      if (dynamic_cast<TimelinePanel*>(tbp)) {
        // Prefer our CloseSequence function which will delete any unnecessary timeline panels
        CloseSequence(static_cast<Sequence*>(tbp->GetConnectedViewer()));
      } else {
        tbp->DisconnectViewerNode();
      }
    }
  }

  // Close any extra folder panels
  foreach (ProjectPanel* panel, folder_panels_) {
    if (panel->project() == p) {
      panel->close();
    }
  }

  // Close project from project panel
  foreach (ProjectPanel* panel, project_panels_) {
    if (panel->project() == p) {
      RemoveProjectPanel(panel);
    }
  }

  // Close project from NodeView
  node_panel_->CloseContextsBelongingToProject(p);
}

void MainWindow::SetApplicationProgressStatus(ProgressStatus status)
{
#if defined(Q_OS_WINDOWS)
  if (taskbar_interface_) {
    switch (status) {
    case kProgressShow:
      taskbar_interface_->SetProgressState(reinterpret_cast<HWND>(this->winId()), TBPF_NORMAL);
      break;
    case kProgressNone:
      taskbar_interface_->SetProgressState(reinterpret_cast<HWND>(this->winId()), TBPF_NOPROGRESS);
      break;
    case kProgressError:
      taskbar_interface_->SetProgressState(reinterpret_cast<HWND>(this->winId()), TBPF_ERROR);
      break;
    }
  }

#elif defined(Q_OS_MAC)
#endif
}

void MainWindow::SetApplicationProgressValue(int value)
{
#if defined(Q_OS_WINDOWS)
  if (taskbar_interface_) {
    taskbar_interface_->SetProgressValue(reinterpret_cast<HWND>(this->winId()), value, 100);
  }
#elif defined(Q_OS_MAC)
#endif
}

void MainWindow::closeEvent(QCloseEvent *e)
{
  // Try to close all projects (this will return false if the user chooses not to close)
  if (!Core::instance()->CloseAllProjects(false)) {
    e->ignore();
    return;
  }

  scope_panel_->SetViewerPanel(nullptr);

  PanelManager::instance()->DeleteAllPanels();

  SaveCustomShortcuts();

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

void MainWindow::NodeGroupRequested(NodeGroup *group)
{
  NodePanel *panel = new NodePanel(this);
  panel->setFloating(true);
  panel->setVisible(true);
  panel->SetContexts({group});
  panel->SetSignalInsteadOfClose(true);
  addDockWidget(Qt::LeftDockWidgetArea, panel);
  connect(panel, &NodePanel::CloseRequested, panel, &NodePanel::deleteLater);
}

void MainWindow::TimelinePanelSelectionChanged(const QVector<Block *> &blocks)
{
  TimelinePanel *panel = static_cast<TimelinePanel *>(sender());

  if (PanelManager::instance()->CurrentlyFocused(false) == panel) {
    UpdateNodePanelContextFromTimelinePanel(panel);
  }
}

void MainWindow::ShowWelcomeDialog()
{
  if (Config::Current()[QStringLiteral("ShowWelcomeDialog")].toBool()) {
    AboutDialog ad(true, this);
    ad.exec();
  }
}

#ifdef Q_OS_LINUX
void MainWindow::ShowNouveauWarning()
{
  QMessageBox::warning(this,
                       tr("Driver Warning"),
                       tr("Olive has detected your system is using the Nouveau graphics driver.\n\nThis driver is "
                          "known to have stability and performance issues with Olive. It is highly recommended "
                          "you install the proprietary NVIDIA driver before continuing to use Olive."),
                       QMessageBox::Ok);
}
#endif

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

  Core::instance()->CloseProject(p, true);
}

void MainWindow::ViewerCloseRequested()
{
  ViewerPanel* panel = static_cast<ViewerPanel*>(sender());

  if (panel == scope_panel_->GetConnectedViewerPanel()) {
    scope_panel_->SetViewerPanel(sequence_viewer_panel_);
  }

  viewer_panels_.remove(viewer_panels_.key(panel));

  panel->deleteLater();
}

void MainWindow::ViewerWithPanelRemovedFromGraph()
{
  ViewerOutput* vo = static_cast<ViewerOutput*>(sender());
  viewer_panels_.take(vo)->deleteLater();
  disconnect(vo, &ViewerOutput::RemovedFromGraph, this, &MainWindow::ViewerWithPanelRemovedFromGraph);
}

void MainWindow::FloatingPanelCloseRequested()
{
  PanelWidget* panel = static_cast<PanelWidget*>(sender());

  quintptr list_ptr = panel->property("parent_list").value<quintptr>();
  QList<PanelWidget*>* list = reinterpret_cast< QList<PanelWidget*>* >(list_ptr);

  list->removeOne(panel);
  panel->deleteLater();
}

TimelinePanel* MainWindow::AppendTimelinePanel()
{
  TimelinePanel* panel = AppendPanelInternal<TimelinePanel>(timeline_panels_);

  connect(panel, &PanelWidget::CloseRequested, this, &MainWindow::TimelineCloseRequested);
  connect(panel, &TimelinePanel::TimeChanged, curve_panel_, &ParamPanel::SetTime);
  connect(panel, &TimelinePanel::TimeChanged, param_panel_, &ParamPanel::SetTime);
  connect(panel, &TimelinePanel::TimeChanged, sequence_viewer_panel_, &SequenceViewerPanel::SetTime);
  connect(panel, &TimelinePanel::BlockSelectionChanged, this, &MainWindow::TimelinePanelSelectionChanged);
  connect(param_panel_, &ParamPanel::TimeChanged, panel, &TimelinePanel::SetTime);
  connect(curve_panel_, &ParamPanel::TimeChanged, panel, &TimelinePanel::SetTime);
  connect(sequence_viewer_panel_, &SequenceViewerPanel::TimeChanged, panel, &TimelinePanel::SetTime);

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
  panel->ConnectViewerNode(nullptr);

  if (timeline_panels_.size() != 1) {
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
  param_panel_->ConnectViewerNode(viewer);
  curve_panel_->ConnectViewerNode(viewer);
}

QString MainWindow::GetCustomShortcutsFile()
{
  return QDir(FileFunctions::GetConfigurationLocation()).filePath(QStringLiteral("shortcuts"));
}

void LoadCustomShortcutsInternal(QMenu* menu, const QMap<QString, QString>& shortcuts)
{
  QList<QAction*> actions = menu->actions();

  foreach (QAction* a, actions) {
    if (a->menu()) {
      LoadCustomShortcutsInternal(a->menu(), shortcuts);
    } else if (!a->isSeparator()) {
      QString action_id = a->property("id").toString();

      if (shortcuts.contains(action_id)) {
        a->setShortcut(shortcuts.value(action_id));
      }
    }
  }
}

void MainWindow::LoadCustomShortcuts()
{
  QFile shortcut_file(GetCustomShortcutsFile());
  if (shortcut_file.exists() && shortcut_file.open(QFile::ReadOnly)) {
    QMap<QString, QString> shortcuts;

    QString shortcut_str = QString::fromUtf8(shortcut_file.readAll());

    QStringList shortcut_list = shortcut_str.split(QStringLiteral("\n"));

    foreach (const QString& s, shortcut_list) {
      QStringList shortcut_line = s.split(QStringLiteral("\t"));
      if (shortcut_line.size() >= 2) {
        shortcuts.insert(shortcut_line.at(0), shortcut_line.at(1));
      }
    }

    shortcut_file.close();

    if (!shortcuts.isEmpty()) {
      QList<QAction*> menus = menuBar()->actions();

      foreach (QAction* menu, menus) {
        LoadCustomShortcutsInternal(menu->menu(), shortcuts);
      }
    }
  }
}

void SaveCustomShortcutsInternal(QMenu* menu, QMap<QString, QString>* shortcuts)
{
  QList<QAction*> actions = menu->actions();

  foreach (QAction* a, actions) {
    if (a->menu()) {
      SaveCustomShortcutsInternal(a->menu(), shortcuts);
    } else if (!a->isSeparator()) {
      QString default_shortcut = a->property("keydefault").value<QKeySequence>().toString();
      QString current_shortcut = a->shortcut().toString();
      if (current_shortcut != default_shortcut) {
        QString action_id = a->property("id").toString();
        shortcuts->insert(action_id, current_shortcut);
      }
    }
  }
}

void MainWindow::SaveCustomShortcuts()
{
  QMap<QString, QString> shortcuts;
  QList<QAction*> menus = menuBar()->actions();

  foreach (QAction* menu, menus) {
    SaveCustomShortcutsInternal(menu->menu(), &shortcuts);
  }

  QFile shortcut_file(GetCustomShortcutsFile());
  if (shortcuts.isEmpty()) {
    if (shortcut_file.exists()) {
      // No custom shortcuts, remove any existing file
      shortcut_file.remove();
    }
  } else if (shortcut_file.open(QFile::WriteOnly)) {
    for (auto it=shortcuts.cbegin(); it!=shortcuts.cend(); it++) {
      if (it != shortcuts.cbegin()) {
        shortcut_file.write(QStringLiteral("\n").toUtf8());
      }

      shortcut_file.write(it.key().toUtf8());
      shortcut_file.write(QStringLiteral("\t").toUtf8());
      shortcut_file.write(it.value().toUtf8());
    }
    shortcut_file.close();
  } else {
    qCritical() << "Failed to save custom keyboard shortcuts";
  }

}

void MainWindow::UpdateAudioMonitorParams(ViewerOutput *viewer)
{
  if (!audio_monitor_panel_->IsPlaying()) {
    audio_monitor_panel_->SetParams(viewer ? viewer->GetAudioParams() : AudioParams());
  }
}

void MainWindow::UpdateNodePanelContextFromTimelinePanel(TimelinePanel *panel)
{
  // Add selected blocks (if any)
  const QVector<Block*> &blocks = panel->GetSelectedBlocks();
  QVector<Node *> context(blocks.size());
  for (int i=0; i<blocks.size(); i++) {
    context[i] = blocks.at(i);
  }

  // If no selected blocks, set the context to the sequence
  ViewerOutput *viewer = panel->GetConnectedViewer();
  if (viewer && context.isEmpty()) {
    context.append(viewer);
  }

  node_panel_->SetContexts(context);
  param_panel_->SetContexts(context);
}

void MainWindow::FocusedPanelChanged(PanelWidget *panel)
{
  // Update audio monitor panel
  if (TimeBasedPanel* tbp = dynamic_cast<TimeBasedPanel*>(panel)) {
    UpdateAudioMonitorParams(tbp->GetConnectedViewer());
  }

  if (NodePanel *node_panel = dynamic_cast<NodePanel*>(panel)) {
    // Set param view contexts to these
    const QVector<Node*> &new_ctxs = node_panel->GetContexts();

    if (new_ctxs != param_panel_->GetContexts()) {
      bool is_default_node_panel = node_panel == node_panel_;
      param_panel_->SetIgnoreNodeFlags(!is_default_node_panel);
      param_panel_->SetCreateCheckBoxes(is_default_node_panel ? kNoCheckBoxes : kCheckBoxesOnNonConnected);
      param_panel_->SetContexts(node_panel->GetContexts());
    }
  } else if (TimelinePanel* timeline = dynamic_cast<TimelinePanel*>(panel)) {
    // Signal timeline focus
    TimelineFocused(timeline->GetConnectedViewer());

    UpdateNodePanelContextFromTimelinePanel(timeline);
  } else if (ProjectPanel* project = dynamic_cast<ProjectPanel*>(panel)) {
    // Signal project panel focus
    UpdateTitle();
  } else if (ViewerPanelBase *viewer = dynamic_cast<ViewerPanelBase*>(panel)) {
    // Update scopes for viewer
    scope_panel_->SetViewerPanel(viewer);
  }
}

void MainWindow::SetDefaultLayout()
{
  footage_viewer_panel_->show();
  addDockWidget(Qt::TopDockWidgetArea, footage_viewer_panel_);

  param_panel_->show();
  tabifyDockWidget(footage_viewer_panel_, param_panel_);

  node_panel_->show();
  tabifyDockWidget(param_panel_, node_panel_);
  param_panel_->raise();

  curve_panel_->hide();
  curve_panel_->setFloating(true);
  addDockWidget(Qt::TopDockWidgetArea, curve_panel_);

  scope_panel_->hide();
  scope_panel_->setFloating(true);
  addDockWidget(Qt::TopDockWidgetArea, scope_panel_);

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

  resizeDocks({param_panel_, sequence_viewer_panel_},
  {width()/2, width()/2},
              Qt::Horizontal);

  resizeDocks({project_panels_.first(), tool_panel_, timeline_panels_.first(), audio_monitor_panel_},
  {width()/4, 1, width(), 1},
              Qt::Horizontal);

  resizeDocks({node_panel_, project_panels_.first()},
  {height()/2, height()/2},
              Qt::Vertical);
}

void MainWindow::showEvent(QShowEvent *e)
{
  QMainWindow::showEvent(e);

  if (first_show_) {
    QMetaObject::invokeMethod(Core::instance(), "CheckForAutoRecoveries", Qt::QueuedConnection);

#ifdef Q_OS_LINUX
    // Check for nouveau since that driver really doesn't work with Olive
    QOffscreenSurface surface;
    surface.create();
    QOpenGLContext context;
    context.create();
    context.makeCurrent(&surface);
    const char* vendor = reinterpret_cast<const char*>(context.functions()->glGetString(GL_VENDOR));
    qDebug() << "Using graphics driver:" << vendor;
    if (!strcmp(vendor, "nouveau")) {
      QMetaObject::invokeMethod(this, "ShowNouveauWarning", Qt::QueuedConnection);
    }
#endif

    QMetaObject::invokeMethod(this, "ShowWelcomeDialog", Qt::QueuedConnection);

    first_show_ = false;
  }
}

template<typename T>
T *MainWindow::AppendPanelInternal(QList<T*>& list)
{
  T* panel = new T(this);

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
T *MainWindow::AppendFloatingPanelInternal(QList<T *> &list)
{
  T* panel = new T(this);

  panel->setFloating(true);
  panel->show();

  panel->SetSignalInsteadOfClose(true);
  connect(panel, &PanelWidget::CloseRequested, this, &MainWindow::FloatingPanelCloseRequested);

  panel->setProperty("parent_list", reinterpret_cast<quintptr>(&list));

  return panel;
}

}
