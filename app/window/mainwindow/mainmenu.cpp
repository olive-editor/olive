/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "mainmenu.h"

#include <QDesktopServices>
#include <QEvent>
#include <QStyleFactory>

#include "common/timecodefunctions.h"
#include "config/config.h"
#include "core.h"
#include "dialog/actionsearch/actionsearch.h"
#include "dialog/diskcache/diskcachedialog.h"
#include "dialog/task/task.h"
#include "panel/panelmanager.h"
#include "tool/tool.h"
#include "ui/style/style.h"
#include "undo/undostack.h"
#include "widget/menu/menushared.h"
#include "mainwindow.h"

namespace olive {

MainMenu::MainMenu(MainWindow *parent) :
  QMenuBar(parent)
{
  //
  // FILE MENU
  //
  file_menu_ = new Menu(this, this, &MainMenu::FileMenuAboutToShow);
  file_new_menu_ = new Menu(file_menu_);
  MenuShared::instance()->AddItemsForNewMenu(file_new_menu_);
  file_open_item_ = file_menu_->AddItem("openproj", Core::instance(), &Core::OpenProject, tr("Ctrl+O"));
  file_open_recent_menu_ = new Menu(file_menu_);
  file_open_recent_separator_ = file_open_recent_menu_->addSeparator();
  file_open_recent_clear_item_ = file_open_recent_menu_->AddItem("clearopenrecent", Core::instance(), &Core::ClearOpenRecentList);
  file_save_item_ = file_menu_->AddItem("saveproj", Core::instance(), &Core::SaveActiveProject, tr("Ctrl+S"));
  file_save_as_item_ = file_menu_->AddItem("saveprojas", Core::instance(), &Core::SaveActiveProjectAs, tr("Ctrl+Shift+S"));
  file_save_all_item_ = file_menu_->AddItem("saveallproj", Core::instance(), &Core::SaveAllProjects);
  file_menu_->addSeparator();
  file_revert_item_ = file_menu_->AddItem("revert", Core::instance(), &Core::RevertActiveProject, tr("F12"));
  file_menu_->addSeparator();
  file_import_item_ = file_menu_->AddItem("import", Core::instance(), &Core::DialogImportShow, tr("Ctrl+I"));
  file_menu_->addSeparator();
  file_export_menu_ = new Menu(file_menu_);
  file_export_media_item_ = file_export_menu_->AddItem("export", Core::instance(), &Core::DialogExportShow, tr("Ctrl+M"));
  file_menu_->addSeparator();
  file_close_project_item_ = file_menu_->AddItem("closeproj", Core::instance(), &Core::CloseActiveProject);
  file_close_all_projects_item_ = file_menu_->AddItem("closeallproj", Core::instance(), static_cast<bool(Core::*)()>(&Core::CloseAllProjects));
  file_close_all_except_item_ = file_menu_->AddItem("closeallexcept", Core::instance(), &Core::CloseAllExceptActiveProject);
  file_menu_->addSeparator();
  file_project_properties_item_ = file_menu_->AddItem("projectproperties", Core::instance(), &Core::DialogProjectPropertiesShow, tr("Shift+F10"));
  file_menu_->addSeparator();
  file_exit_item_ = file_menu_->AddItem("exit", parent, &MainWindow::close);

  //
  // EDIT MENU
  //
  edit_menu_ = new Menu(this);

  connect(edit_menu_, &Menu::aboutToShow, this, &MainMenu::EditMenuAboutToShow);
  connect(edit_menu_, &Menu::aboutToHide, this, &MainMenu::EditMenuAboutToHide);

  edit_undo_item_ = Core::instance()->undo_stack()->GetUndoAction();
  Menu::ConformItem(edit_undo_item_, "undo", tr("Ctrl+Z"));
  edit_menu_->addAction(edit_undo_item_);
  edit_redo_item_ = Core::instance()->undo_stack()->GetRedoAction();
  Menu::ConformItem(edit_redo_item_, "redo", tr("Ctrl+Shift+Z"));
  edit_menu_->addAction(edit_redo_item_);

  edit_menu_->addSeparator();
  MenuShared::instance()->AddItemsForEditMenu(edit_menu_, true);
  {
    // Create "alternate delete" action so we can pick up backspace as well as delete while still
    // keeping them configurable
    edit_delete2_item_ = new QAction();
    Menu::ConformItem(edit_delete2_item_, "delete2", MenuShared::instance(), &MenuShared::DeleteSelectedTriggered, tr("Backspace"));
    auto actions = edit_menu_->actions();
    edit_menu_->insertAction(actions.at(actions.indexOf(MenuShared::instance()->edit_delete_item()) + 1), edit_delete2_item_);
  }
  edit_menu_->addSeparator();
  edit_select_all_item_ = edit_menu_->AddItem("selectall", this, &MainMenu::SelectAllTriggered, tr("Ctrl+A"));
  edit_deselect_all_item_ = edit_menu_->AddItem("deselectall", this, &MainMenu::DeselectAllTriggered, tr("Ctrl+Shift+A"));
  edit_menu_->addSeparator();
  MenuShared::instance()->AddItemsForClipEditMenu(edit_menu_);
  edit_menu_->addSeparator();
  edit_insert_item_ = edit_menu_->AddItem("insert", this, &MainMenu::InsertTriggered, tr(","));
  edit_overwrite_item_ = edit_menu_->AddItem("overwrite", this, &MainMenu::OverwriteTriggered, tr("."));
  edit_menu_->addSeparator();
  edit_ripple_to_in_item_ = edit_menu_->AddItem("rippletoin", this, &MainMenu::RippleToInTriggered, tr("Q"));
  edit_ripple_to_out_item_ = edit_menu_->AddItem("rippletoout", this, &MainMenu::RippleToOutTriggered, tr("W"));
  edit_edit_to_in_item_ = edit_menu_->AddItem("edittoin", this, &MainMenu::EditToInTriggered, tr("Ctrl+Alt+Q"));
  edit_edit_to_out_item_ = edit_menu_->AddItem("edittoout", this, &MainMenu::EditToOutTriggered, tr("Ctrl+Alt+W"));
  edit_menu_->addSeparator();
  edit_nudge_left_item_ = edit_menu_->AddItem("nudgeleft", this, &MainMenu::NudgeLeftTriggered, tr("Alt+Left"));
  edit_nudge_right_item_ = edit_menu_->AddItem("nudgeright", this, &MainMenu::NudgeRightTriggered, tr("Alt+Right"));
  edit_move_in_to_playhead_item_ = edit_menu_->AddItem("moveintoplayhead", this, &MainMenu::MoveInToPlayheadTriggered, tr("["));
  edit_move_out_to_playhead_item_ = edit_menu_->AddItem("moveouttoplayhead", this, &MainMenu::MoveOutToPlayheadTriggered, tr("]"));
  edit_menu_->addSeparator();
  MenuShared::instance()->AddItemsForInOutMenu(edit_menu_);
  edit_delete_inout_item_ = edit_menu_->AddItem("deleteinout", this, &MainMenu::DeleteInOutTriggered, tr(";"));
  edit_ripple_delete_inout_item_ = edit_menu_->AddItem("rippledeleteinout", this, &MainMenu::RippleDeleteInOutTriggered, tr("'"));
  edit_menu_->addSeparator();
  edit_set_marker_item_ = edit_menu_->AddItem("marker", this, &MainMenu::SetMarkerTriggered, tr("M"));

  //
  // VIEW MENU
  //
  view_menu_ = new Menu(this, this, &MainMenu::ViewMenuAboutToShow);
  view_zoom_in_item_ = view_menu_->AddItem("zoomin", this, &MainMenu::ZoomInTriggered, tr("="));
  view_zoom_out_item_ = view_menu_->AddItem("zoomout", this, &MainMenu::ZoomOutTriggered, tr("-"));
  view_increase_track_height_item_ = view_menu_->AddItem("vzoomin", this, &MainMenu::IncreaseTrackHeightTriggered, tr("Ctrl+="));
  view_decrease_track_height_item_ = view_menu_->AddItem("vzoomout", this, &MainMenu::DecreaseTrackHeightTriggered, tr("Ctrl+-"));
  view_show_all_item_ = view_menu_->AddItem("showall", this, &MainMenu::ToggleShowAllTriggered, tr("\\"));
  view_show_all_item_->setCheckable(true);

  view_menu_->addSeparator();

  view_full_screen_item_ = view_menu_->AddItem("fullscreen", parent, &MainWindow::SetFullscreen, tr("F11"));
  view_full_screen_item_->setCheckable(true);

  view_full_screen_viewer_item_ = view_menu_->AddItem("fullscreenviewer", this, &MainMenu::FullScreenViewerTriggered);

  //
  // PLAYBACK MENU
  //
  playback_menu_ = new Menu(this, this, &MainMenu::PlaybackMenuAboutToShow);
  playback_gotostart_item_ = playback_menu_->AddItem("gotostart", this, &MainMenu::GoToStartTriggered, tr("Home"));
  playback_prevframe_item_ = playback_menu_->AddItem("prevframe", this, &MainMenu::PrevFrameTriggered, tr("Left"));
  playback_playpause_item_ = playback_menu_->AddItem("playpause", this, &MainMenu::PlayPauseTriggered, tr("Space"));
  playback_playinout_item_ = playback_menu_->AddItem("playintoout", this, &MainMenu::PlayInToOutTriggered, tr("Shift+Space"));
  playback_nextframe_item_ = playback_menu_->AddItem("nextframe", this, &MainMenu::NextFrameTriggered, tr("Right"));
  playback_gotoend_item_ = playback_menu_->AddItem("gotoend", this, &MainMenu::GoToEndTriggered, tr("End"));

  playback_menu_->addSeparator();

  playback_prevcut_item_ = playback_menu_->AddItem("prevcut", this, &MainMenu::GoToPrevCutTriggered, tr("Up"));
  playback_nextcut_item_ = playback_menu_->AddItem("nextcut", this, &MainMenu::GoToNextCutTriggered, tr("Down"));

  playback_menu_->addSeparator();

  playback_gotoin_item_ = playback_menu_->AddItem("gotoin", this, &MainMenu::GoToInTriggered, tr("Shift+I"));
  playback_gotoout_item_ = playback_menu_->AddItem("gotoout", this, &MainMenu::GoToOutTriggered, tr("Shift+O"));

  playback_menu_->addSeparator();

  playback_shuttleleft_item_ = playback_menu_->AddItem("decspeed", this, &MainMenu::ShuttleLeftTriggered, tr("J"));
  playback_shuttlestop_item_ = playback_menu_->AddItem("pause", this, &MainMenu::ShuttleStopTriggered, tr("K"));
  playback_shuttleright_item_ = playback_menu_->AddItem("incspeed", this, &MainMenu::ShuttleRightTriggered, tr("L"));

  playback_menu_->addSeparator();

  playback_loop_item_ = playback_menu_->AddItem("loop", this, &MainMenu::LoopTriggered);
  playback_loop_item_->setCheckable(true);

  //
  // SEQUENCE MENU
  //

  sequence_menu_ = new Menu(this, this, &MainMenu::SequenceMenuAboutToShow);
  sequence_cache_item_ = sequence_menu_->AddItem("seqcache", this, &MainMenu::SequenceCacheTriggered);
  sequence_cache_in_to_out_item_ = sequence_menu_->AddItem("seqcacheinout", this, &MainMenu::SequenceCacheInOutTriggered);

  sequence_menu_->addSeparator();

  sequence_disk_cache_clear_item_ = sequence_menu_->AddItem("seqcacheclear", this, &MainMenu::SequenceCacheClearTriggered);

  // TEMP: Hide sequence cache items for now. Want to see if clip caching will supersede it.
  sequence_cache_item_->setVisible(false);
  sequence_cache_in_to_out_item_->setVisible(false);

  //
  // WINDOW MENU
  //
  window_menu_ = new Menu(this, this, &MainMenu::WindowMenuAboutToShow);
  window_menu_separator_ = window_menu_->addSeparator();
  window_maximize_panel_item_ = window_menu_->AddItem("maximizepanel", parent, &MainWindow::ToggleMaximizedPanel, tr("`"));
  window_lock_layout_item_ = window_menu_->AddItem("lockpanels", PanelManager::instance(), &PanelManager::SetPanelsLocked);
  window_lock_layout_item_->setCheckable(true);
  window_menu_->addSeparator();
  window_reset_layout_item_ = window_menu_->AddItem("resetdefaultlayout", parent, &MainWindow::SetDefaultLayout);

  //
  // TOOLS MENU
  //
  tools_menu_ = new Menu(this, this, &MainMenu::ToolsMenuAboutToShow);
  tools_menu_->setToolTipsVisible(true);

  tools_group_ = new QActionGroup(this);

  tools_pointer_item_ = tools_menu_->AddItem("pointertool", this, &MainMenu::ToolItemTriggered, tr("V"));
  tools_pointer_item_->setCheckable(true);
  tools_pointer_item_->setData(Tool::kPointer);
  tools_group_->addAction(tools_pointer_item_);

  tools_trackselect_item_ = tools_menu_->AddItem("trackselecttool", this, &MainMenu::ToolItemTriggered, tr("D"));
  tools_trackselect_item_->setCheckable(true);
  tools_trackselect_item_->setData(Tool::kTrackSelect);
  tools_group_->addAction(tools_trackselect_item_);

  tools_edit_item_ = tools_menu_->AddItem("edittool", this, &MainMenu::ToolItemTriggered, tr("X"));
  tools_edit_item_->setCheckable(true);
  tools_edit_item_->setData(Tool::kEdit);
  tools_group_->addAction(tools_edit_item_);

  tools_ripple_item_ = tools_menu_->AddItem("rippletool", this, &MainMenu::ToolItemTriggered, tr("B"));
  tools_ripple_item_->setCheckable(true);
  tools_ripple_item_->setData(Tool::kRipple);
  tools_group_->addAction(tools_ripple_item_);

  tools_rolling_item_ = tools_menu_->AddItem("rollingtool", this, &MainMenu::ToolItemTriggered, tr("N"));
  tools_rolling_item_->setCheckable(true);
  tools_rolling_item_->setData(Tool::kRolling);
  tools_group_->addAction(tools_rolling_item_);

  tools_razor_item_ = tools_menu_->AddItem("razortool", this, &MainMenu::ToolItemTriggered, tr("C"));
  tools_razor_item_->setCheckable(true);
  tools_razor_item_->setData(Tool::kRazor);
  tools_group_->addAction(tools_razor_item_);

  tools_slip_item_ = tools_menu_->AddItem("sliptool", this, &MainMenu::ToolItemTriggered, tr("Y"));
  tools_slip_item_->setCheckable(true);
  tools_slip_item_->setData(Tool::kSlip);
  tools_group_->addAction(tools_slip_item_);

  tools_slide_item_ = tools_menu_->AddItem("slidetool", this, &MainMenu::ToolItemTriggered, tr("U"));
  tools_slide_item_->setCheckable(true);
  tools_slide_item_->setData(Tool::kSlide);
  tools_group_->addAction(tools_slide_item_);

  tools_hand_item_ = tools_menu_->AddItem("handtool", this, &MainMenu::ToolItemTriggered, tr("H"));
  tools_hand_item_->setCheckable(true);
  tools_hand_item_->setData(Tool::kHand);
  tools_group_->addAction(tools_hand_item_);

  tools_zoom_item_ = tools_menu_->AddItem("zoomtool", this, &MainMenu::ToolItemTriggered, tr("Z"));
  tools_zoom_item_->setCheckable(true);
  tools_zoom_item_->setData(Tool::kZoom);
  tools_group_->addAction(tools_zoom_item_);

  tools_transition_item_ = tools_menu_->AddItem("transitiontool", this, &MainMenu::ToolItemTriggered, tr("T"));
  tools_transition_item_->setCheckable(true);
  tools_transition_item_->setData(Tool::kTransition);
  tools_group_->addAction(tools_transition_item_);

  tools_add_item_ = tools_menu_->AddItem("addtool", this, &MainMenu::ToolItemTriggered, tr("A"));
  tools_add_item_->setCheckable(true);
  tools_add_item_->setData(Tool::kAdd);
  tools_group_->addAction(tools_add_item_);

  tools_record_item_ = tools_menu_->AddItem("recordtool", this, &MainMenu::ToolItemTriggered, tr("R"));
  tools_record_item_->setCheckable(true);
  tools_record_item_->setData(Tool::kRecord);
  tools_group_->addAction(tools_record_item_);

  tools_menu_->addSeparator();

  tools_snapping_item_ = tools_menu_->AddItem("snapping", Core::instance(), &Core::SetSnapping, tr("S"));
  tools_snapping_item_->setCheckable(true);
  tools_snapping_item_->setChecked(Core::instance()->snapping());

  tools_menu_->addSeparator();

  tools_preferences_item_ = tools_menu_->AddItem("prefs", Core::instance(), &Core::DialogPreferencesShow, tr("Ctrl+,"));

  //
  // HELP MENU
  //
  help_menu_ = new Menu(this);
  help_action_search_item_ = help_menu_->AddItem("actionsearch", this, &MainMenu::ActionSearchTriggered, tr("/"));
  help_menu_->addSeparator();
  help_feedback_item_ = help_menu_->AddItem("feedback", this, &MainMenu::HelpFeedbackTriggered);
  help_menu_->addSeparator();
  help_about_item_ = help_menu_->AddItem("about", Core::instance(), &Core::DialogAboutShow);

  connect(Core::instance(), &Core::OpenRecentListChanged, this, &MainMenu::RepopulateOpenRecent);
  PopulateOpenRecent();

  Retranslate();
}

void MainMenu::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  QMenuBar::changeEvent(e);
}

void MainMenu::ToolItemTriggered()
{
  // Assume the sender is a QAction
  QAction* action = static_cast<QAction*>(sender());

  // Assume its data() is a member of Tool::Item
  Tool::Item tool = static_cast<Tool::Item>(action->data().toInt());

  // Set the Tool in Core
  Core::instance()->SetTool(tool);
}

void MainMenu::FileMenuAboutToShow()
{
  Project* active_project = Core::instance()->GetActiveProject();

  file_save_item_->setEnabled(active_project);
  file_save_as_item_->setEnabled(active_project);
  file_close_project_item_->setEnabled(active_project);
  file_close_all_projects_item_->setEnabled(active_project);
  file_close_all_except_item_->setEnabled(active_project);

  if (active_project) {
    file_save_item_->setText(tr("&Save '%1'").arg(active_project->name()));
    file_save_as_item_->setText(tr("Save '%1' &As").arg(active_project->name()));
    file_close_project_item_->setText(tr("Close '%1'").arg(active_project->name()));
    file_close_all_except_item_->setText(tr("Close All Except '%1'").arg(active_project->name()));
  } else {
    file_save_item_->setText(tr("&Save Project"));
    file_save_as_item_->setText(tr("Save Project &As"));
    file_close_project_item_->setText(tr("Close Project"));
    file_close_all_except_item_->setText(tr("Close All Except Current Project"));
  }
}

void MainMenu::EditMenuAboutToShow()
{
  edit_delete2_item_->setVisible(false);
}

void MainMenu::EditMenuAboutToHide()
{
  edit_delete2_item_->setVisible(true);
}

void MainMenu::ViewMenuAboutToShow()
{
  // Parent is QMainWindow
  view_full_screen_item_->setChecked(parentWidget()->isFullScreen());

  // Make sure we're displaying the correct options for the timebase
  TimeBasedPanel* p = PanelManager::instance()->MostRecentlyFocused<TimeBasedPanel>();
  if (p) {
    if (p->timebase().denominator() != 0) {
      view_menu_->addSeparator();
      MenuShared::instance()->AddItemsForTimeRulerMenu(view_menu_);
    }
  }

  // Ensure checked timecode display mode is correct
  MenuShared::instance()->AboutToShowTimeRulerActions(p->timebase());
}

void MainMenu::ToolsMenuAboutToShow()
{
  // Ensure checked Tool is correct
  QList<QAction*> tool_actions = tools_group_->actions();
  foreach (QAction* a, tool_actions) {
    if (a->data() == Core::instance()->tool()) {
      a->setChecked(true);
      break;
    }
  }

  // Ensure snapping value is correct
  tools_snapping_item_->setChecked(Core::instance()->snapping());
}

void MainMenu::PlaybackMenuAboutToShow()
{
  playback_loop_item_->setChecked(OLIVE_CONFIG("Loop").toBool());
}

void MainMenu::SequenceMenuAboutToShow()
{
  TimeBasedPanel* p = PanelManager::instance()->MostRecentlyFocused<TimeBasedPanel>();

  bool can_cache_sequence = (p && p->GetConnectedViewer());

  sequence_cache_item_->setEnabled(can_cache_sequence);
  sequence_cache_in_to_out_item_->setEnabled(can_cache_sequence);
}

void MainMenu::WindowMenuAboutToShow()
{
  // Remove any previous items
  while (window_menu_->actions().first() != window_menu_separator_) {
    window_menu_->removeAction(window_menu_->actions().first());
  }

  QList<QAction*> panel_actions;

  // Alphabetize actions - keeps actions in a consistent order since PanelManager::panels() is
  // ordered from most recently focused to least, which may be confusing user experience.
  foreach (PanelWidget* panel, PanelManager::instance()->panels()) {
    QAction* panel_action = panel->toggleViewAction();

    bool inserted = false;

    for (int i=0;i<panel_actions.size();i++) {
      if (panel_actions.at(i)->text() > panel_action->text()) {
        panel_actions.insert(i, panel_action);
        inserted = true;
        break;
      }
    }

    if (!inserted) {
      panel_actions.append(panel_action);
    }
  }

  // Add new items
  window_menu_->insertActions(window_menu_separator_, panel_actions);

  window_lock_layout_item_->setChecked(PanelManager::instance()->ArePanelsLocked());
}

void MainMenu::PopulateOpenRecent()
{
  if (Core::instance()->GetRecentProjects().isEmpty()) {

    // Insert dummy/disabled action to show there's nothing
    QAction* a = new QAction(tr("(None)"));
    a->setEnabled(false);
    file_open_recent_menu_->insertAction(file_open_recent_separator_, a);

  } else {

    // Populate menu with recently opened projects
    for (int i=0;i<Core::instance()->GetRecentProjects().size();i++) {
      QAction* a = new QAction(Core::instance()->GetRecentProjects().at(i));
      a->setData(i);
      connect(a, &QAction::triggered, this, &MainMenu::OpenRecentItemTriggered);
      file_open_recent_menu_->insertAction(file_open_recent_separator_, a);
    }

  }
}

void MainMenu::RepopulateOpenRecent()
{
  CloseOpenRecentMenu();
  PopulateOpenRecent();
}

void MainMenu::CloseOpenRecentMenu()
{
  while (file_open_recent_menu_->actions().first() != file_open_recent_separator_) {
    file_open_recent_menu_->removeAction(file_open_recent_menu_->actions().first());
  }
}

void MainMenu::ZoomInTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->ZoomIn();
}

void MainMenu::ZoomOutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->ZoomOut();
}

void MainMenu::IncreaseTrackHeightTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->IncreaseTrackHeight();
}

void MainMenu::DecreaseTrackHeightTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->DecreaseTrackHeight();
}

void MainMenu::GoToStartTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->GoToStart();
}

void MainMenu::PrevFrameTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->PrevFrame();
}

void MainMenu::PlayPauseTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->PlayPause();
}

void MainMenu::PlayInToOutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->PlayInToOut();
}

void MainMenu::LoopTriggered(bool enabled)
{
  OLIVE_CONFIG("Loop") = enabled;
}

void MainMenu::NextFrameTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->NextFrame();
}

void MainMenu::GoToEndTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->GoToEnd();
}

void MainMenu::SelectAllTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->SelectAll();
}

void MainMenu::DeselectAllTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->DeselectAll();
}

void MainMenu::InsertTriggered()
{
  FootageManagementPanel* project_panel = PanelManager::instance()->MostRecentlyFocused<FootageManagementPanel>();
  TimelinePanel *timeline_panel = PanelManager::instance()->MostRecentlyFocused<TimelinePanel>();

  if (project_panel && timeline_panel) {
    timeline_panel->InsertFootageAtPlayhead(project_panel->GetSelectedFootage());
  }
}

void MainMenu::OverwriteTriggered()
{
  FootageManagementPanel* project_panel = PanelManager::instance()->MostRecentlyFocused<FootageManagementPanel>();
  TimelinePanel *timeline_panel = PanelManager::instance()->MostRecentlyFocused<TimelinePanel>();

  if (project_panel && timeline_panel) {
    timeline_panel->OverwriteFootageAtPlayhead(project_panel->GetSelectedFootage());
  }
}

void MainMenu::RippleToInTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->RippleToIn();
}

void MainMenu::RippleToOutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->RippleToOut();
}

void MainMenu::EditToInTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->EditToIn();
}

void MainMenu::EditToOutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->EditToOut();
}

void MainMenu::NudgeLeftTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->NudgeLeft();
}

void MainMenu::NudgeRightTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->NudgeRight();
}

void MainMenu::MoveInToPlayheadTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->MoveInToPlayhead();
}

void MainMenu::MoveOutToPlayheadTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->MoveOutToPlayhead();
}

void MainMenu::ActionSearchTriggered()
{
  ActionSearch as(parentWidget());
  as.SetMenuBar(this);
  as.exec();
}

void MainMenu::ShuttleLeftTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->ShuttleLeft();
}

void MainMenu::ShuttleStopTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->ShuttleStop();
}

void MainMenu::ShuttleRightTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->ShuttleRight();
}

void MainMenu::GoToPrevCutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->GoToPrevCut();
}

void MainMenu::GoToNextCutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->GoToNextCut();
}

void MainMenu::SetMarkerTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->SetMarker();
}

void MainMenu::FullScreenViewerTriggered()
{
  PanelManager::instance()->MostRecentlyFocused<ViewerPanel>()->SetFullScreen();
}

void MainMenu::ToggleShowAllTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->ToggleShowAll();
}

void MainMenu::DeleteInOutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->DeleteInToOut();
}

void MainMenu::RippleDeleteInOutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->RippleDeleteInToOut();
}

void MainMenu::GoToInTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->GoToIn();
}

void MainMenu::GoToOutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->GoToOut();
}

void MainMenu::OpenRecentItemTriggered()
{
  Core::instance()->OpenProjectFromRecentList(static_cast<QAction*>(sender())->data().toInt());
}

void MainMenu::SequenceCacheTriggered()
{
  Core::instance()->CacheActiveSequence(false);
}

void MainMenu::SequenceCacheInOutTriggered()
{
  Core::instance()->CacheActiveSequence(true);
}

void MainMenu::SequenceCacheClearTriggered()
{
  DiskCacheDialog::ClearDiskCache(
        Core::instance()->GetActiveProject()->cache_path(),
        Core::instance()->main_window()
        );
}

void MainMenu::HelpFeedbackTriggered()
{
  QDesktopServices::openUrl(QStringLiteral("https://github.com/olive-editor/olive/issues"));
}

void MainMenu::Retranslate()
{
  // MenuShared is not a QWidget and therefore does not receive a LanguageEvent, we use MainMenu's to update it
  MenuShared::instance()->Retranslate();

  // File menu
  file_menu_->setTitle(tr("&File"));
  file_new_menu_->setTitle(tr("&New"));
  file_open_item_->setText(tr("&Open Project"));
  file_open_recent_menu_->setTitle(tr("Open &Recent"));
  file_open_recent_clear_item_->setText(tr("&Clear Recent List"));
  file_save_all_item_->setText(tr("Sa&ve All Projects"));
  file_revert_item_->setText(tr("Revert"));
  file_import_item_->setText(tr("&Import..."));
  file_export_menu_->setTitle(tr("&Export"));
  file_export_media_item_->setText(tr("&Media..."));
  file_close_all_projects_item_->setText(tr("Close All Projects"));
  file_project_properties_item_->setText(tr("Project Properties"));
  file_exit_item_->setText(tr("E&xit"));

  // Edit menu
  edit_menu_->setTitle(tr("&Edit"));
  Core::instance()->undo_stack()->UpdateActions(); // Update undo and redo
  edit_delete2_item_->setText(tr("Delete (alt)"));
  edit_insert_item_->setText(tr("Insert"));
  edit_overwrite_item_->setText(tr("Overwrite"));
  edit_select_all_item_->setText(tr("Select &All"));
  edit_deselect_all_item_->setText(tr("Deselect All"));
  edit_ripple_to_in_item_->setText(tr("Ripple to In Point"));
  edit_ripple_to_out_item_->setText(tr("Ripple to Out Point"));
  edit_edit_to_in_item_->setText(tr("Edit to In Point"));
  edit_edit_to_out_item_->setText(tr("Edit to Out Point"));
  edit_nudge_left_item_->setText(tr("Nudge Left"));
  edit_nudge_right_item_->setText(tr("Nudge Right"));
  edit_move_in_to_playhead_item_->setText(tr("Move In Point to Playhead"));
  edit_move_out_to_playhead_item_->setText(tr("Move Out Point to Playhead"));
  edit_delete_inout_item_->setText(tr("Delete In/Out Point"));
  edit_ripple_delete_inout_item_->setText(tr("Ripple Delete In/Out Point"));
  edit_set_marker_item_->setText(tr("Set/Edit Marker"));

  // View menu
  view_menu_->setTitle(tr("&View"));
  view_zoom_in_item_->setText(tr("Zoom In"));
  view_zoom_out_item_->setText(tr("Zoom Out"));
  view_increase_track_height_item_->setText(tr("Increase Track Height"));
  view_decrease_track_height_item_->setText(tr("Decrease Track Height"));
  view_show_all_item_->setText(tr("Toggle Show All"));

  // View menu (cont'd)
  view_full_screen_item_->setText(tr("Full Screen"));
  view_full_screen_viewer_item_->setText(tr("Full Screen Viewer"));

  // Playback menu
  playback_menu_->setTitle(tr("&Playback"));
  playback_gotostart_item_->setText(tr("Go to Start"));
  playback_prevframe_item_->setText(tr("Previous Frame"));
  playback_playpause_item_->setText(tr("Play/Pause"));
  playback_playinout_item_->setText(tr("Play In to Out"));
  playback_nextframe_item_->setText(tr("Next Frame"));
  playback_gotoend_item_->setText(tr("Go to End"));
  playback_prevcut_item_->setText(tr("Go to Previous Cut"));
  playback_nextcut_item_->setText(tr("Go to Next Cut"));
  playback_gotoin_item_->setText(tr("Go to In Point"));
  playback_gotoout_item_->setText(tr("Go to Out Point"));
  playback_shuttleleft_item_->setText(tr("Shuttle Left"));
  playback_shuttlestop_item_->setText(tr("Shuttle Stop"));
  playback_shuttleright_item_->setText(tr("Shuttle Right"));
  playback_loop_item_->setText(tr("Loop"));

  // Sequence menu
  sequence_menu_->setTitle(tr("&Sequence"));
  sequence_cache_item_->setText(tr("Cache Entire Sequence"));
  sequence_cache_in_to_out_item_->setText(tr("Cache Sequence In/Out"));
  sequence_disk_cache_clear_item_->setText(tr("Clear Disk Cache"));

  // Window menu
  window_menu_->setTitle(tr("&Window"));
  window_maximize_panel_item_->setText(tr("Maximize Panel"));
  window_lock_layout_item_->setText(tr("Lock Panels"));
  window_reset_layout_item_->setText(tr("Reset to Default Layout"));

  // Tools menu
  tools_menu_->setTitle(tr("&Tools"));
  tools_pointer_item_->setText(tr("Pointer Tool"));
  tools_trackselect_item_->setText(tr("Track Select Tool"));
  tools_edit_item_->setText(tr("Edit Tool"));
  tools_ripple_item_->setText(tr("Ripple Tool"));
  tools_rolling_item_->setText(tr("Rolling Tool"));
  tools_razor_item_->setText(tr("Razor Tool"));
  tools_slip_item_->setText(tr("Slip Tool"));
  tools_slide_item_->setText(tr("Slide Tool"));
  tools_hand_item_->setText(tr("Hand Tool"));
  tools_zoom_item_->setText(tr("Zoom Tool"));
  tools_transition_item_->setText(tr("Transition Tool"));
  tools_add_item_->setText(tr("Add Tool"));
  tools_record_item_->setText(tr("Record Tool"));
  tools_snapping_item_->setText(tr("Enable Snapping"));
  tools_preferences_item_->setText(tr("Preferences"));

  // Help menu
  help_menu_->setTitle(tr("&Help"));
  help_action_search_item_->setText(tr("A&ction Search"));
  help_feedback_item_->setText(tr("Send &Feedback..."));
  help_about_item_->setText(tr("&About..."));
}

}
