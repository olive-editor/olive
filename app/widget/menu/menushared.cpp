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

#include "menushared.h"

#include <QActionGroup>

#include "core.h"
#include "panel/panelmanager.h"
#include "panel/timeline/timeline.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

MenuShared* MenuShared::instance_ = nullptr;

MenuShared::MenuShared()
{
  // "New" menu shared items
  new_project_item_ = Menu::CreateItem(this, "newproj", Core::instance(), &Core::CreateNewProject, tr("Ctrl+N"));
  new_sequence_item_ = Menu::CreateItem(this, "newseq", Core::instance(), &Core::CreateNewSequence, tr("Ctrl+Shift+N"));
  new_folder_item_ = Menu::CreateItem(this, "newfolder", Core::instance(), &Core::CreateNewFolder);

  // "Edit" menu shared items
  edit_cut_item_ = Menu::CreateItem(this, "cut", this, &MenuShared::CutTriggered, tr("Ctrl+X"));
  edit_copy_item_ = Menu::CreateItem(this, "copy", this, &MenuShared::CopyTriggered, tr("Ctrl+C"));
  edit_paste_item_ = Menu::CreateItem(this, "paste", this, &MenuShared::PasteTriggered, tr("Ctrl+V"));
  edit_paste_insert_item_ = Menu::CreateItem(this, "pasteinsert", this, &MenuShared::PasteInsertTriggered, tr("Ctrl+Shift+V"));
  edit_duplicate_item_ = Menu::CreateItem(this, "duplicate", this, &MenuShared::DuplicateTriggered, tr("Ctrl+D"));
  edit_rename_item_ = Menu::CreateItem(this, "rename", this, &MenuShared::RenameSelectedTriggered, tr("F2"));
  edit_delete_item_ = Menu::CreateItem(this, "delete", this, &MenuShared::DeleteSelectedTriggered, tr("Del"));
  edit_ripple_delete_item_ = Menu::CreateItem(this, "rippledelete", this, &MenuShared::RippleDeleteTriggered, tr("Shift+Del"));
  edit_split_item_ = Menu::CreateItem(this, "split", this, &MenuShared::SplitAtPlayheadTriggered, tr("Ctrl+K"));
  edit_speedduration_item_ = Menu::CreateItem(this, "speeddur", this, &MenuShared::SpeedDurationTriggered, tr("Ctrl+R"));

  // List of addable items
  for (int i=0;i<Tool::kAddableCount;i++) {
    Tool::AddableObject t = static_cast<Tool::AddableObject>(i);
    QAction *a = Menu::CreateItem(this, QStringLiteral("add:%1").arg(Tool::GetAddableObjectID(t)), this, &MenuShared::AddableItemTriggered);
    a->setData(t);
    addable_items_.append(a);
  }

  // "In/Out" menu shared items
  inout_set_in_item_ = Menu::CreateItem(this, "setinpoint", this, &MenuShared::SetInTriggered, tr("I"));
  inout_set_out_item_ = Menu::CreateItem(this, "setoutpoint", this, &MenuShared::SetOutTriggered, tr("O"));
  inout_reset_in_item_ = Menu::CreateItem(this, "resetin", this, &MenuShared::ResetInTriggered);
  inout_reset_out_item_ = Menu::CreateItem(this, "resetout", this, &MenuShared::ResetOutTriggered);
  inout_clear_inout_item_ = Menu::CreateItem(this, "clearinout", this, &MenuShared::ClearInOutTriggered, tr("G"));

  // "Clip Edit" menu shared items
  clip_add_default_transition_item_ = Menu::CreateItem(this, "deftransition", this, &MenuShared::DefaultTransitionTriggered, tr("Ctrl+Shift+D"));
  clip_link_unlink_item_ = Menu::CreateItem(this, "linkunlink", this, &MenuShared::ToggleLinksTriggered, tr("Ctrl+L"));
  clip_enable_disable_item_ = Menu::CreateItem(this, "enabledisable", this, &MenuShared::EnableDisableTriggered, tr("Shift+E"));
  clip_nest_item_ = Menu::CreateItem(this, "nest", this, &MenuShared::NestTriggered);

  // TimeRuler menu shared items
  frame_view_mode_group_ = new QActionGroup(this);

  view_timecode_view_dropframe_item_ = Menu::CreateItem(this, "modedropframe", this, &MenuShared::TimecodeDisplayTriggered);
  view_timecode_view_dropframe_item_->setData(Timecode::kTimecodeDropFrame);
  view_timecode_view_dropframe_item_->setCheckable(true);
  frame_view_mode_group_->addAction(view_timecode_view_dropframe_item_);

  view_timecode_view_nondropframe_item_ = Menu::CreateItem(this, "modenondropframe", this, &MenuShared::TimecodeDisplayTriggered);
  view_timecode_view_nondropframe_item_->setData(Timecode::kTimecodeNonDropFrame);
  view_timecode_view_nondropframe_item_->setCheckable(true);
  frame_view_mode_group_->addAction(view_timecode_view_nondropframe_item_);

  view_timecode_view_seconds_item_ = Menu::CreateItem(this, "modeseconds", this, &MenuShared::TimecodeDisplayTriggered);
  view_timecode_view_seconds_item_->setData(Timecode::kTimecodeSeconds);
  view_timecode_view_seconds_item_->setCheckable(true);
  frame_view_mode_group_->addAction(view_timecode_view_seconds_item_);

  view_timecode_view_frames_item_ = Menu::CreateItem(this, "modeframes", this, &MenuShared::TimecodeDisplayTriggered);
  view_timecode_view_frames_item_->setData(Timecode::kFrames);
  view_timecode_view_frames_item_->setCheckable(true);
  frame_view_mode_group_->addAction(view_timecode_view_frames_item_);

  view_timecode_view_milliseconds_item_ = Menu::CreateItem(this, "milliseconds", this, &MenuShared::TimecodeDisplayTriggered);
  view_timecode_view_milliseconds_item_->setData(Timecode::kMilliseconds);
  view_timecode_view_milliseconds_item_->setCheckable(true);
  frame_view_mode_group_->addAction(view_timecode_view_milliseconds_item_);

  // Color coding menu items
  color_coding_menu_ = new ColorLabelMenu();
  connect(color_coding_menu_, &ColorLabelMenu::ColorSelected, this, &MenuShared::ColorLabelTriggered);

  Retranslate();
}

MenuShared::~MenuShared()
{
  delete color_coding_menu_;
}

void MenuShared::CreateInstance()
{
  instance_ = new MenuShared();
}

void MenuShared::DestroyInstance()
{
  delete instance_;
}

void MenuShared::AddItemsForNewMenu(Menu *m)
{
  m->addAction(new_project_item_);
  m->addSeparator();
  m->addAction(new_sequence_item_);
  m->addAction(new_folder_item_);
}

void MenuShared::AddItemsForEditMenu(Menu *m, bool for_clips)
{
  m->addAction(Core::instance()->undo_stack()->GetUndoAction());
  m->addAction(Core::instance()->undo_stack()->GetRedoAction());

  m->addSeparator();

  m->addAction(edit_cut_item_);
  m->addAction(edit_copy_item_);
  m->addAction(edit_paste_item_);
  m->addAction(edit_paste_insert_item_);
  m->addAction(edit_duplicate_item_);
  m->addAction(edit_rename_item_);
  m->addAction(edit_delete_item_);

  if (for_clips) {
    m->addAction(edit_ripple_delete_item_);
    m->addAction(edit_split_item_);
    m->addAction(edit_speedduration_item_);

    m->addSeparator();

    m->addAction(clip_add_default_transition_item_);
    m->addAction(clip_link_unlink_item_);
    m->addAction(clip_enable_disable_item_);
    m->addAction(clip_nest_item_);
  }
}

void MenuShared::AddItemsForAddableObjectsMenu(Menu *m)
{
  for (QAction *a : qAsConst(addable_items_)) {
    a->setChecked((a->data().toInt() == Core::instance()->GetSelectedAddableObject()));
    m->addAction(a);
  }
}

void MenuShared::AddItemsForInOutMenu(Menu *m)
{
  m->addAction(inout_set_in_item_);
  m->addAction(inout_set_out_item_);
  m->addSeparator();
  m->addAction(inout_reset_in_item_);
  m->addAction(inout_reset_out_item_);
  m->addAction(inout_clear_inout_item_);
}

void MenuShared::AddColorCodingMenu(Menu *m)
{
  m->addMenu(color_coding_menu_);
}

void MenuShared::AddItemsForClipEditMenu(Menu *m)
{
  m->addAction(clip_add_default_transition_item_);
  m->addAction(clip_link_unlink_item_);
  m->addAction(clip_enable_disable_item_);
  m->addAction(clip_nest_item_);
}

void MenuShared::AddItemsForTimeRulerMenu(Menu *m)
{
  m->addAction(view_timecode_view_dropframe_item_);
  m->addAction(view_timecode_view_nondropframe_item_);
  m->addAction(view_timecode_view_seconds_item_);
  m->addAction(view_timecode_view_frames_item_);
  m->addAction(view_timecode_view_milliseconds_item_);
}

void MenuShared::AboutToShowTimeRulerActions(const rational& timebase)
{
  QList<QAction*> timecode_display_actions = frame_view_mode_group_->actions();
  Timecode::Display current_timecode_display = Core::instance()->GetTimecodeDisplay();

  // Only show the drop-frame option if the timebase is drop-frame
  view_timecode_view_dropframe_item_->setVisible(!timebase.isNull() && Timecode::timebase_is_drop_frame(timebase));

  if (!view_timecode_view_dropframe_item_->isVisible() && current_timecode_display == Timecode::kTimecodeDropFrame) {
    // If the current setting is drop-frame, correct to non-drop frame
    current_timecode_display = Timecode::kTimecodeNonDropFrame;
  }

  foreach (QAction* a, timecode_display_actions) {
    if (a->data() == current_timecode_display) {
      a->setChecked(true);
      break;
    }
  }
}

MenuShared *MenuShared::instance()
{
  return instance_;
}

void MenuShared::SplitAtPlayheadTriggered()
{
  TimelinePanel* timeline = PanelManager::instance()->MostRecentlyFocused<TimelinePanel>();

  if (timeline != nullptr) {
    timeline->SplitAtPlayhead();
  }
}

void MenuShared::DeleteSelectedTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->DeleteSelected();
}

void MenuShared::RippleDeleteTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->RippleDelete();
}

void MenuShared::SetInTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->SetIn();
}

void MenuShared::SetOutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->SetOut();
}

void MenuShared::ResetInTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->ResetIn();
}

void MenuShared::ResetOutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->ResetOut();
}

void MenuShared::ClearInOutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->ClearInOut();
}

void MenuShared::ToggleLinksTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->ToggleLinks();
}

void MenuShared::CutTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->CutSelected();
}

void MenuShared::CopyTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->CopySelected();
}

void MenuShared::PasteTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->Paste();
}

void MenuShared::PasteInsertTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->PasteInsert();
}

void MenuShared::DuplicateTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->Duplicate();
}

void MenuShared::RenameSelectedTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->RenameSelected();
}

void MenuShared::EnableDisableTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->ToggleSelectedEnabled();
}

void MenuShared::NestTriggered()
{
  PanelManager::instance()->MostRecentlyFocused<TimelinePanel>()->NestSelectedClips();
}

void MenuShared::DefaultTransitionTriggered()
{
  PanelManager::instance()->MostRecentlyFocused<TimelinePanel>()->AddDefaultTransitionsToSelected();
}

void MenuShared::TimecodeDisplayTriggered()
{
  // Assume the sender is a QAction
  QAction* action = static_cast<QAction*>(sender());

  // Assume its data() is a member of Timecode::Display
  Timecode::Display display = static_cast<Timecode::Display>(action->data().toInt());

  // Set the current display mode
  Core::instance()->SetTimecodeDisplay(display);
}

void MenuShared::ColorLabelTriggered(int color_index)
{
  PanelManager::instance()->CurrentlyFocused()->SetColorLabel(color_index);
}

void MenuShared::SpeedDurationTriggered()
{
  TimelinePanel* timeline = PanelManager::instance()->MostRecentlyFocused<TimelinePanel>();

  if (timeline) {
    timeline->ShowSpeedDurationDialogForSelectedClips();
  }
}

void MenuShared::AddableItemTriggered()
{
  QAction *a = static_cast<QAction*>(sender());
  Tool::AddableObject i = static_cast<Tool::AddableObject>(a->data().toInt());
  Core::instance()->SetTool(Tool::kAdd);
  Core::instance()->SetSelectedAddableObject(i);
}

void MenuShared::Retranslate()
{
  // "New" menu shared items
  new_project_item_->setText(tr("&Project"));
  new_sequence_item_->setText(tr("&Sequence"));
  new_folder_item_->setText(tr("&Folder"));

  // "Edit" menu shared items
  edit_cut_item_->setText(tr("Cu&t"));
  edit_copy_item_->setText(tr("Cop&y"));
  edit_paste_item_->setText(tr("&Paste"));
  edit_paste_insert_item_->setText(tr("Paste Insert"));
  edit_duplicate_item_->setText(tr("Duplicate"));
  edit_rename_item_->setText(tr("Rename"));
  edit_delete_item_->setText(tr("Delete"));
  edit_ripple_delete_item_->setText(tr("Ripple Delete"));
  edit_split_item_->setText(tr("Split"));
  edit_speedduration_item_->setText(tr("Speed/Duration"));

  for (QAction *a : qAsConst(addable_items_)) {
    a->setText(Tool::GetAddableObjectName(static_cast<Tool::AddableObject>(a->data().toInt())));
  }

  // "In/Out" menu shared items
  inout_set_in_item_->setText(tr("Set In Point"));
  inout_set_out_item_->setText(tr("Set Out Point"));
  inout_reset_in_item_->setText(tr("Reset In Point"));
  inout_reset_out_item_->setText(tr("Reset Out Point"));
  inout_clear_inout_item_->setText(tr("Clear In/Out Point"));

  // "Clip Edit" menu shared items
  clip_add_default_transition_item_->setText(tr("Add Default Transition"));
  clip_link_unlink_item_->setText(tr("Link/Unlink"));
  clip_enable_disable_item_->setText(tr("Enable/Disable"));
  clip_nest_item_->setText(tr("Nest"));

  // TimeRuler menu shared items
  view_timecode_view_frames_item_->setText(tr("Frames"));
  view_timecode_view_dropframe_item_->setText(tr("Drop Frame"));
  view_timecode_view_nondropframe_item_->setText(tr("Non-Drop Frame"));
  view_timecode_view_milliseconds_item_->setText(tr("Milliseconds"));
  view_timecode_view_seconds_item_->setText(tr("Seconds"));
}

}
