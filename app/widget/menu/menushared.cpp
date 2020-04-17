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

#include "menushared.h"

#include "core.h"
#include "panel/panelmanager.h"
#include "panel/timeline/timeline.h"

OLIVE_NAMESPACE_ENTER

MenuShared* MenuShared::instance_ = nullptr;

MenuShared::MenuShared()
{
  // "New" menu shared items
  new_project_item_ = Menu::CreateItem(this, "newproj", Core::instance(), &Core::CreateNewProject, "Ctrl+N");
  new_sequence_item_ = Menu::CreateItem(this, "newseq", Core::instance(), &Core::CreateNewSequence, "Ctrl+Shift+N");
  new_folder_item_ = Menu::CreateItem(this, "newfolder", Core::instance(), &Core::CreateNewFolder);

  // "Edit" menu shared items
  edit_cut_item_ = Menu::CreateItem(this, "cut", this, &MenuShared::CutTriggered, "Ctrl+X");
  edit_copy_item_ = Menu::CreateItem(this, "copy", this, &MenuShared::CopyTriggered, "Ctrl+C");
  edit_paste_item_ = Menu::CreateItem(this, "paste", this, &MenuShared::PasteTriggered, "Ctrl+V");
  edit_paste_insert_item_ = Menu::CreateItem(this, "pasteinsert", this, &MenuShared::PasteInsertTriggered, "Ctrl+Shift+V");
  edit_duplicate_item_ = Menu::CreateItem(this, "duplicate", this, &MenuShared::DuplicateTriggered, "Ctrl+D");
  edit_delete_item_ = Menu::CreateItem(this, "delete", this, &MenuShared::DeleteSelectedTriggered, "Del");
  edit_ripple_delete_item_ = Menu::CreateItem(this, "rippledelete", this, &MenuShared::RippleDeleteTriggered, "Shift+Del");
  edit_split_item_ = Menu::CreateItem(this, "split", this, &MenuShared::SplitAtPlayheadTriggered, "Ctrl+K");

  // "In/Out" menu shared items
  inout_set_in_item_ = Menu::CreateItem(this, "setinpoint", this, &MenuShared::SetInTriggered, "I");
  inout_set_out_item_ = Menu::CreateItem(this, "setoutpoint", this, &MenuShared::SetOutTriggered, "O");
  inout_reset_in_item_ = Menu::CreateItem(this, "resetin", this, &MenuShared::ResetInTriggered);
  inout_reset_out_item_ = Menu::CreateItem(this, "resetout", this, &MenuShared::ResetOutTriggered);
  inout_clear_inout_item_ = Menu::CreateItem(this, "clearinout", this, &MenuShared::ClearInOutTriggered, "G");

  // "Clip Edit" menu shared items
  clip_add_default_transition_item_ = Menu::CreateItem(this, "deftransition", this, &MenuShared::DefaultTransitionTriggered, "Ctrl+Shift+D");
  clip_link_unlink_item_ = Menu::CreateItem(this, "linkunlink", this, &MenuShared::ToggleLinksTriggered, "Ctrl+L");
  clip_enable_disable_item_ = Menu::CreateItem(this, "enabledisable", this, &MenuShared::EnableDisableTriggered, "Shift+E");
  clip_nest_item_ = Menu::CreateItem(this, "nest", this, &MenuShared::NestTriggered);

  Retranslate();
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

void MenuShared::AddItemsForEditMenu(Menu *m)
{
  m->addAction(edit_cut_item_);
  m->addAction(edit_copy_item_);
  m->addAction(edit_paste_item_);
  m->addAction(edit_paste_insert_item_);
  m->addAction(edit_duplicate_item_);
  m->addAction(edit_delete_item_);
  m->addAction(edit_ripple_delete_item_);
  m->addAction(edit_split_item_);
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

void MenuShared::AddItemsForClipEditMenu(Menu *m)
{
  m->addAction(clip_add_default_transition_item_);
  m->addAction(clip_link_unlink_item_);
  m->addAction(clip_enable_disable_item_);
  m->addAction(clip_nest_item_);
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
  qDebug() << "FIXME: Stub";
}

void MenuShared::EnableDisableTriggered()
{
  PanelManager::instance()->CurrentlyFocused()->ToggleSelectedEnabled();
}

void MenuShared::NestTriggered()
{
  qDebug() << "FIXME: Stub";
}

void MenuShared::DefaultTransitionTriggered()
{
  qDebug() << "FIXME: Stub";
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
  edit_delete_item_->setText(tr("Delete"));
  edit_ripple_delete_item_->setText(tr("Ripple Delete"));
  edit_split_item_->setText(tr("Split"));

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
}

OLIVE_NAMESPACE_EXIT
