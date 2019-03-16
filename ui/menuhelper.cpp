/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#include "menuhelper.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <QStyleFactory>

#include "global/config.h"
#include "project/clipboard.h"
#include "ui/mainwindow.h"
#include "global/global.h"
#include "panels/panels.h"
#include "ui/focusfilter.h"

MenuHelper olive::MenuHelper;

void MenuHelper::InitializeSharedMenus()
{
  new_project_ = create_menu_action(nullptr, "newproj", olive::Global.get(), SLOT(new_project()), QKeySequence("Ctrl+N"));
  new_project_->setParent(this);

  new_sequence_ = create_menu_action(nullptr, "newseq", panel_project, SLOT(new_sequence()), QKeySequence("Ctrl+Shift+N"));
  new_sequence_->setParent(this);

  new_folder_ = create_menu_action(nullptr, "newfolder", panel_project, SLOT(new_folder()));
  new_folder_->setParent(this);

  set_in_point_ = create_menu_action(nullptr, "setinpoint", &olive::FocusFilter, SLOT(set_in_point()), QKeySequence("I"));
  set_in_point_->setParent(this);

  set_out_point_ = create_menu_action(nullptr, "setoutpoint", &olive::FocusFilter, SLOT(set_out_point()), QKeySequence("O"));
  set_out_point_->setParent(this);

  reset_in_point_ = create_menu_action(nullptr, "resetin", &olive::FocusFilter, SLOT(clear_in()));
  reset_in_point_->setParent(this);

  reset_out_point_ = create_menu_action(nullptr, "resetout", &olive::FocusFilter, SLOT(clear_out()));
  reset_out_point_->setParent(this);

  clear_inout_point = create_menu_action(nullptr, "clearinout", &olive::FocusFilter, SLOT(clear_inout()), QKeySequence("G"));
  clear_inout_point->setParent(this);

  add_default_transition_ = create_menu_action(nullptr, "deftransition", panel_timeline, SLOT(add_transition()), QKeySequence("Ctrl+Shift+D"));
  add_default_transition_->setParent(this);

  link_unlink_ = create_menu_action(nullptr, "linkunlink", panel_timeline, SLOT(toggle_links()), QKeySequence("Ctrl+L"));
  link_unlink_->setParent(this);

  enable_disable_ = create_menu_action(nullptr, "enabledisable", panel_timeline, SLOT(toggle_enable_on_selected_clips()), QKeySequence("Shift+E"));
  enable_disable_->setParent(this);

  nest_ = create_menu_action(nullptr, "nest", panel_timeline, SLOT(nest()));
  nest_->setParent(this);

  cut_ = create_menu_action(nullptr, "cut", &olive::FocusFilter, SLOT(cut()), QKeySequence("Ctrl+X"));
  cut_->setParent(this);

  copy_ = create_menu_action(nullptr, "copy", &olive::FocusFilter, SLOT(copy()), QKeySequence("Ctrl+C"));
  copy_->setParent(this);

  paste_ = create_menu_action(nullptr, "paste", olive::Global.get(), SLOT(paste()), QKeySequence("Ctrl+V"));
  paste_->setParent(this);

  paste_insert_ = create_menu_action(nullptr, "pasteinsert", olive::Global.get(), SLOT(paste_insert()), QKeySequence("Ctrl+Shift+V"));
  paste_insert_->setParent(this);

  duplicate_ = create_menu_action(nullptr, "duplicate", &olive::FocusFilter, SLOT(duplicate()), QKeySequence("Ctrl+D"));
  duplicate_->setParent(this);

  delete_ = create_menu_action(nullptr, "delete", &olive::FocusFilter, SLOT(delete_function()), QKeySequence("Del"));
  delete_->setParent(this);

  ripple_delete_ = create_menu_action(nullptr, "rippledelete", panel_timeline, SLOT(ripple_delete()), QKeySequence("Shift+Del"));
  ripple_delete_->setParent(this);

  split_ = create_menu_action(nullptr, "split", panel_timeline, SLOT(split_at_playhead()), QKeySequence("Ctrl+K"));
  split_->setParent(this);

  Retranslate();
}

void MenuHelper::make_new_menu(QMenu *parent) {
  parent->addAction(new_project_);
  parent->addSeparator();
  parent->addAction(new_sequence_);
  parent->addAction(new_folder_);
}

void MenuHelper::make_inout_menu(QMenu *parent) {
  parent->addAction(set_in_point_);
  parent->addAction(set_out_point_);
  parent->addSeparator();
  parent->addAction(reset_in_point_);
  parent->addAction(reset_out_point_);
  parent->addAction(clear_inout_point);
}

void MenuHelper::make_clip_functions_menu(QMenu *parent) {
  parent->addAction(add_default_transition_);
  parent->addAction(link_unlink_);
  parent->addAction(enable_disable_);
  parent->addAction(nest_);
}

void MenuHelper::make_edit_functions_menu(QMenu *parent, bool objects_are_selected) {
  if (objects_are_selected) {
    parent->addAction(cut_);
    parent->addAction(copy_);
  }

  parent->addAction(paste_);
  parent->addAction(paste_insert_);

  if (objects_are_selected) {
    parent->addAction(duplicate_);
    parent->addAction(delete_);
    parent->addAction(ripple_delete_);
    parent->addAction(split_);
  }
}

void MenuHelper::set_bool_action_checked(QAction *a) {
  if (!a->data().isNull()) {
    bool* variable = reinterpret_cast<bool*>(a->data().value<quintptr>());
    a->setChecked(*variable);
  }
}

void MenuHelper::set_int_action_checked(QAction *a, const int& i) {
  if (!a->data().isNull()) {
    a->setChecked(a->data() == i);
  }
}

void MenuHelper::set_button_action_checked(QAction *a) {
  a->setChecked(reinterpret_cast<QPushButton*>(a->data().value<quintptr>())->isChecked());
}

void MenuHelper::Retranslate()
{
  new_project_->setText(tr("&Project"));
  new_sequence_->setText(tr("&Sequence"));
  new_folder_->setText(tr("&Folder"));
  set_in_point_->setText(tr("Set In Point"));
  set_out_point_->setText(tr("Set Out Point"));
  reset_in_point_->setText(tr("Reset In Point"));
  reset_out_point_->setText(tr("Reset Out Point"));
  clear_inout_point->setText(tr("Clear In/Out Point"));
  add_default_transition_->setText(tr("Add Default Transition"));
  link_unlink_->setText(tr("Link/Unlink"));
  enable_disable_->setText(tr("Enable/Disable"));
  nest_->setText(tr("Nest"));
  cut_->setText(tr("Cu&t"));
  copy_->setText(tr("Cop&y"));
  paste_->setText(tr("&Paste"));
  paste_insert_->setText(tr("Paste Insert"));
  duplicate_->setText(tr("Duplicate"));
  delete_->setText(tr("Delete"));
  ripple_delete_->setText(tr("Ripple Delete"));
  split_->setText(tr("Split"));
}

void MenuHelper::toggle_bool_action() {
  QAction* action = static_cast<QAction*>(sender());
  bool* variable = reinterpret_cast<bool*>(action->data().value<quintptr>());
  *variable = !(*variable);
  update_ui(false);
}

void MenuHelper::set_titlesafe_from_menu() {
  double tsa = static_cast<QAction*>(sender())->data().toDouble();

  if (qIsNaN(tsa)) {

    // disable title safe area
    olive::CurrentConfig.show_title_safe_area = false;

  } else {

    // using title safe area
    olive::CurrentConfig.show_title_safe_area = true;

    // are we using the default area aspect ratio, or a specific one
    if (qIsNull(tsa)) {

      // default title safe area
      olive::CurrentConfig.use_custom_title_safe_ratio = false;

    } else {

      // using a specific aspect ratio
      olive::CurrentConfig.use_custom_title_safe_ratio = true;

      if (tsa < 0.0) {

        // set a custom title safe area
        QString input;
        bool invalid = false;
        QRegExp arTest("[0-9.]+:[0-9.]+");

        do {
          if (invalid) {
            QMessageBox::critical(olive::MainWindow, tr("Invalid aspect ratio"), tr("The aspect ratio '%1' is invalid. Please try again.").arg(input));
          }

          input = QInputDialog::getText(olive::MainWindow, tr("Enter custom aspect ratio"), tr("Enter the aspect ratio to use for the title/action safe area (e.g. 16:9):"));
          invalid = !arTest.exactMatch(input) && !input.isEmpty();
        } while (invalid);

        if (!input.isEmpty()) {
          QStringList inputList = input.split(':');
          olive::CurrentConfig.custom_title_safe_ratio = inputList.at(0).toDouble()/inputList.at(1).toDouble();
        }

      } else {

        // specified tsa is a specific custom aspect ratio
        olive::CurrentConfig.custom_title_safe_ratio = tsa;
      }

    }

  }

  panel_sequence_viewer->viewer_widget->update();
}

void MenuHelper::set_autoscroll() {
  QAction* action = static_cast<QAction*>(sender());
  olive::CurrentConfig.autoscroll = action->data().toInt();
}

void MenuHelper::menu_click_button() {
  reinterpret_cast<QPushButton*>(static_cast<QAction*>(sender())->data().value<quintptr>())->click();
}

void MenuHelper::set_timecode_view() {
  QAction* action = static_cast<QAction*>(sender());
  olive::CurrentConfig.timecode_view = action->data().toInt();
  update_ui(false);
}

void MenuHelper::open_recent_from_menu() {
  int index = static_cast<QAction*>(sender())->data().toInt();
  olive::Global.get()->open_recent(index);
}

void MenuHelper::create_effect_paste_action(QMenu *menu)
{
  QAction* paste_action = menu->addAction(tr("&Paste"), panel_timeline, SLOT(paste(bool)));
  paste_action->setEnabled(clipboard.size() > 0 && clipboard_type == CLIPBOARD_TYPE_EFFECT);
}

Menu* MenuHelper::create_submenu(QMenuBar* parent,
                                  const QObject *receiver,
                                  const char *member) {
  Menu* menu = new Menu(parent);

  /*
  menu->setStyle(QStyleFactory::create("windowsvista"));
  menu->setPalette(menu->style()->standardPalette());
  menu->setStyleSheet("");
  */

  parent->addMenu(menu);

  if (receiver != nullptr) {
    QObject::connect(menu, SIGNAL(aboutToShow()), receiver, member);
  }

  return menu;
}

Menu* MenuHelper::create_submenu(QMenu* parent) {
  Menu* menu = new Menu(parent);

  /*
  menu->setStyle(QStyleFactory::create("windowsvista"));
  menu->setPalette(menu->style()->standardPalette());
  menu->setStyleSheet("");
  */

  parent->addMenu(menu);
  return menu;
}

QAction* MenuHelper::create_menu_action(QWidget *parent,
                                        const char* id,
                                        const QObject *receiver,
                                        const char *member,
                                        const QKeySequence &shortcut) {
  QAction* action = new QAction(parent);
  action->setProperty("id", id);
  action->setShortcut(shortcut);

  if (receiver != nullptr) {
    QObject::connect(action, SIGNAL(triggered(bool)), receiver, member);
  }

  if (parent != nullptr) {
    parent->addAction(action);
  }

  return action;
}
