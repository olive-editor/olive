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

#include "preferenceskeyboardtab.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

OLIVE_NAMESPACE_ENTER

PreferencesKeyboardTab::PreferencesKeyboardTab(QMenuBar *menubar)
{
  QVBoxLayout* shortcut_layout = new QVBoxLayout(this);
  shortcut_layout->setMargin(0);

  QLineEdit* key_search_line = new QLineEdit();
  key_search_line->setPlaceholderText(tr("Search for action or shortcut"));
  connect(key_search_line, SIGNAL(textChanged(const QString &)), this, SLOT(refine_shortcut_list(const QString &)));

  shortcut_layout->addWidget(key_search_line);

  keyboard_tree_ = new QTreeWidget();
  QTreeWidgetItem* tree_header = keyboard_tree_->headerItem();
  tree_header->setText(0, tr("Action"));
  tree_header->setText(1, tr("Shortcut"));
  shortcut_layout->addWidget(keyboard_tree_);

  QHBoxLayout* reset_shortcut_layout = new QHBoxLayout();

  QPushButton* import_shortcut_button = new QPushButton(tr("Import"));
  reset_shortcut_layout->addWidget(import_shortcut_button);
  connect(import_shortcut_button, SIGNAL(clicked(bool)), this, SLOT(load_shortcut_file()));

  QPushButton* export_shortcut_button = new QPushButton(tr("Export"));
  reset_shortcut_layout->addWidget(export_shortcut_button);
  connect(export_shortcut_button, SIGNAL(clicked(bool)), this, SLOT(save_shortcut_file()));

  reset_shortcut_layout->addStretch();

  QPushButton* reset_selected_shortcut_button = new QPushButton(tr("Reset Selected"));
  reset_shortcut_layout->addWidget(reset_selected_shortcut_button);
  connect(reset_selected_shortcut_button, SIGNAL(clicked(bool)), this, SLOT(reset_default_shortcut()));

  shortcut_layout->addLayout(reset_shortcut_layout);

  setup_kbd_shortcuts(menubar);
}

void PreferencesKeyboardTab::Accept()
{
  // Save keyboard shortcuts
  for (int i=0;i<key_shortcut_fields_.size();i++) {
    key_shortcut_fields_.at(i)->set_action_shortcut();
  }
}

void PreferencesKeyboardTab::setup_kbd_shortcuts(QMenuBar* menubar) {
  QList<QAction*> menus = menubar->actions();

  for (int i=0;i<menus.size();i++) {
    QMenu* menu = menus.at(i)->menu();

    QTreeWidgetItem* item = new QTreeWidgetItem(keyboard_tree_);
    item->setText(0, menu->title().replace("&", ""));

    keyboard_tree_->addTopLevelItem(item);

    setup_kbd_shortcut_worker(menu, item);
  }

  for (int i=0;i<key_shortcut_items_.size();i++) {
    if (!key_shortcut_actions_.at(i)->property("id").isNull()) {
      KeySequenceEditor* editor = new KeySequenceEditor(keyboard_tree_, key_shortcut_actions_.at(i));
      keyboard_tree_->setItemWidget(key_shortcut_items_.at(i), 1, editor);
      key_shortcut_fields_.append(editor);
    }
  }
}

void PreferencesKeyboardTab::setup_kbd_shortcut_worker(QMenu* menu, QTreeWidgetItem* parent) {
  QList<QAction*> actions = menu->actions();
  for (int i=0;i<actions.size();i++) {
    QAction* a = actions.at(i);

    if (!a->isSeparator() && a->property("keyignore").isNull()) {
      QTreeWidgetItem* item = new QTreeWidgetItem(parent);
      item->setText(0, a->text().replace("&", ""));

      parent->addChild(item);

      if (a->menu() != nullptr) {
        item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        setup_kbd_shortcut_worker(a->menu(), item);
      } else {
        key_shortcut_items_.append(item);
        key_shortcut_actions_.append(a);
      }
    }
  }
}

void PreferencesKeyboardTab::reset_default_shortcut() {
  QList<QTreeWidgetItem*> items = keyboard_tree_->selectedItems();
  for (int i=0;i<items.size();i++) {
    QTreeWidgetItem* item = keyboard_tree_->selectedItems().at(i);
    static_cast<KeySequenceEditor*>(keyboard_tree_->itemWidget(item, 1))->reset_to_default();
  }
}

bool PreferencesKeyboardTab::refine_shortcut_list(const QString &s, QTreeWidgetItem* parent) {
  if (parent == nullptr) {
    for (int i=0;i<keyboard_tree_->topLevelItemCount();i++) {
      refine_shortcut_list(s, keyboard_tree_->topLevelItem(i));
    }
  } else {
    parent->setExpanded(!s.isEmpty());

    bool all_children_are_hidden = !s.isEmpty();

    for (int i=0;i<parent->childCount();i++) {
      QTreeWidgetItem* item = parent->child(i);
      if (item->childCount() > 0) {
        all_children_are_hidden = refine_shortcut_list(s, item);
      } else {
        item->setHidden(false);
        if (s.isEmpty()) {
          all_children_are_hidden = false;
        } else {
          QString shortcut;
          if (keyboard_tree_->itemWidget(item, 1) != nullptr) {
            shortcut = static_cast<QKeySequenceEdit*>(keyboard_tree_->itemWidget(item, 1))->keySequence().toString();
          }
          if (item->text(0).contains(s, Qt::CaseInsensitive) || shortcut.contains(s, Qt::CaseInsensitive)) {
            all_children_are_hidden = false;
          } else {
            item->setHidden(true);
          }
        }
      }
    }

    if (parent->text(0).contains(s, Qt::CaseInsensitive)) all_children_are_hidden = false;

    parent->setHidden(all_children_are_hidden);

    return all_children_are_hidden;
  }
  return true;
}

void PreferencesKeyboardTab::load_shortcut_file() {
  QString fn = QFileDialog::getOpenFileName(this, tr("Import Keyboard Shortcuts"));
  if (!fn.isEmpty()) {
    QFile f(fn);
    if (f.exists() && f.open(QFile::ReadOnly)) {
      QByteArray ba = f.readAll();
      f.close();
      for (int i=0;i<key_shortcut_fields_.size();i++) {
        int index = ba.indexOf(key_shortcut_fields_.at(i)->action_name());
        if (index == 0 || (index > 0 && ba.at(index-1) == '\n')) {
          while (index < ba.size() && ba.at(index) != '\t') index++;
          QString ks;
          index++;
          while (index < ba.size() && ba.at(index) != '\n') {
            ks.append(ba.at(index));
            index++;
          }
          key_shortcut_fields_.at(i)->setKeySequence(ks);
        } else {
          key_shortcut_fields_.at(i)->reset_to_default();
        }
      }
    } else {
      QMessageBox::critical(
            this,
            tr("Error saving shortcuts"),
            tr("Failed to open file for reading")
            );
    }
  }
}

void PreferencesKeyboardTab::save_shortcut_file() {
  QString fn = QFileDialog::getSaveFileName(this, tr("Export Keyboard Shortcuts"));
  if (!fn.isEmpty()) {
    QFile f(fn);
    if (f.open(QFile::WriteOnly)) {
      bool start = true;
      for (int i=0;i<key_shortcut_fields_.size();i++) {
        QString s = key_shortcut_fields_.at(i)->export_shortcut();
        if (!s.isEmpty()) {
          if (!start) f.write("\n");
          f.write(s.toUtf8());
          start = false;
        }
      }
      f.close();
      QMessageBox::information(this, tr("Export Shortcuts"), tr("Shortcuts exported successfully"));
    } else {
      QMessageBox::critical(this, tr("Error saving shortcuts"), tr("Failed to open file for writing"));
    }
  }
}

// Keyboard shortcuts are not stored in the config file so. hence we don't ResetValuesFromConfig
// like the other preferences tab
void PreferencesKeyboardTab::ResetDefaults(bool reset_all_tabs)
{
  bool confirm_reset = true;
  if (!reset_all_tabs) {
    confirm_reset = QMessageBox::question(this, tr("Confirm Reset Keyboard Settings"),
                                          tr("Are you sure you wish to reset all Keyboard settings?"),
                                          QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
  }
  if (confirm_reset) {
    for (int i = 0; i < key_shortcut_fields_.size(); i++) {
      key_shortcut_fields_.at(i)->reset_to_default();
    }
  }
}

OLIVE_NAMESPACE_EXIT
