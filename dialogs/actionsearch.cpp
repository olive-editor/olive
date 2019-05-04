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

#include "actionsearch.h"

#include <QVBoxLayout>
#include <QKeyEvent>
#include <QMenuBar>
#include <QLabel>

#include "ui/mainwindow.h"

ActionSearch::ActionSearch(QWidget *parent) :
  QDialog(parent)
{
  // ActionSearch requires a parent widget
  Q_ASSERT(parent != nullptr);

  // Set styling (object name is required for CSS specific to this object)
  setObjectName("ASDiag");
  setStyleSheet("#ASDiag{border: 2px solid #808080;}");

  // Size proportionally to the parent (usually MainWindow).
  resize(parent->width()/3, parent->height()/3);

  // Show dialog as a "popup", which will make the dialog close if the user clicks out of it.
  setWindowFlags(Qt::Popup);

  QVBoxLayout* layout = new QVBoxLayout(this);

  // Construct the main entry text field.
  ActionSearchEntry* entry_field = new ActionSearchEntry(this);

  // Set the main entry field font size to 1.2x its standard font size.
  QFont entry_field_font = entry_field->font();
  entry_field_font.setPointSize(qRound(entry_field_font.pointSize()*1.2));
  entry_field->setFont(entry_field_font);

  // Set placeholder text for the main entry field
  entry_field->setPlaceholderText(tr("Search for action..."));

  // Connect signals/slots
  connect(entry_field, SIGNAL(textChanged(const QString&)), this, SLOT(search_update(const QString &)));
  connect(entry_field, SIGNAL(returnPressed()), this, SLOT(perform_action()));

  // moveSelectionUp() and moveSelectionDown() are emitted when the user pressed up or down on the text field.
  // We override it here to select the upper or lower item in the list.
  connect(entry_field, SIGNAL(moveSelectionUp()), this, SLOT(move_selection_up()));
  connect(entry_field, SIGNAL(moveSelectionDown()), this, SLOT(move_selection_down()));
  layout->addWidget(entry_field);

  // Construct list of actions
  list_widget = new ActionSearchList(this);

  // Set list's font to 1.2x its standard font size
  QFont list_widget_font = list_widget->font();
  list_widget_font.setPointSize(qRound(list_widget_font.pointSize()*1.2));
  list_widget->setFont(list_widget_font);

  layout->addWidget(list_widget);

  connect(list_widget, SIGNAL(dbl_click()), this, SLOT(perform_action()));

  // Instantly focus on the entry field to allow for fully keyboard operation (if this popup was initiated by keyboard
  // shortcut for example).
  entry_field->setFocus();
}

void ActionSearch::search_update(const QString &s, const QString &p, QMenu *parent) {

  // This function is recursive, using the `parent` parameter to loop through a menu's items. It functions in two
  // modes - the parent being NULL, meaning it'll get MainWindow's menubar and loop over its menus, and the parent
  // referring to a menu at which point it'll loop over its actions (and call itself recursively if it finds any
  // submenus).

  if (parent == nullptr) {

    // If parent is NULL, we'll pull from the MainWindow's menubar and call this recursively on all of its submenus
    // (and their submenus).

    // We'll clear all the current items in the list since if we're here, we're just starting.
    list_widget->clear();

    QList<QAction*> menus = olive::MainWindow->menuBar()->actions();

    // Loop through all menus from the menubar and run this function on each one.
    for (int i=0;i<menus.size();i++) {
      QMenu* menu = menus.at(i)->menu();

      search_update(s, p, menu);
    }

    // Once we're here, all the recursion/item retrieval is complete. We auto-select the first item for better
    // keyboard-exclusive functionality.
    if (list_widget->count() > 0) {
      list_widget->item(0)->setSelected(true);
    }

  } else {

    // Parent was not NULL, so we loop over the actions in the menu we were given in `parent`.

    // The list shows a '>' delimited hierarchy of the menus in which this action came from. We construct it here by
    // adding the current menu's text to the existing hierarchy (passed in `p`).
    QString menu_text;
    if (!p.isEmpty()) menu_text += p + " > ";
    menu_text += parent->title().replace("&", ""); // Strip out any &s used in menu action names

    // Loop over the menu's actions
    QList<QAction*> actions = parent->actions();
    for (int i=0;i<actions.size();i++) {

      QAction* a = actions.at(i);

      // Ignore separator actions
      if (!a->isSeparator()) {

        if (a->menu() != nullptr) {

          // If the action is a menu, run this function recursively on it
          search_update(s, menu_text, a->menu());

        } else {

          // This is a valid non-separator non-menu action, so check it against the currently entered string.

          // Strip out all &s from the action's name
          QString comp = a->text().replace("&", "");

          // See if the action's name contains any of the currently entered string
          if (comp.contains(s, Qt::CaseInsensitive)) {

            // If so, we add it to the list widget.
            QListWidgetItem* item = new QListWidgetItem(QString("%1\n(%2)").arg(comp, menu_text), list_widget);

            // Add a pointer to the original QAction in the item's data
            item->setData(Qt::UserRole+1, reinterpret_cast<quintptr>(a));

            list_widget->addItem(item);

          }

        }
      }
    }
  }
}

void ActionSearch::perform_action() {

  // Loop over all the items in the list and if we find one that's selected, we trigger it.
  QList<QListWidgetItem*> selected_items = list_widget->selectedItems();
  if (list_widget->count() > 0 && selected_items.size() > 0) {

    QListWidgetItem* item = selected_items.at(0);

    // Get QAction pointer from item's data
    QAction* a = reinterpret_cast<QAction*>(item->data(Qt::UserRole+1).value<quintptr>());

    a->trigger();

  }

  // Close this popup
  accept();

}

void ActionSearch::move_selection_up() {

  // Here we loop over all the items to find the currently selected one, and then select the one above it. We start
  // iterating at 1 (instead of 0) to efficiently ignore the first item (since the selection can't go below the very
  // bottom item).

  int lim = list_widget->count();
  for (int i=1;i<lim;i++) {
    if (list_widget->item(i)->isSelected()) {
      list_widget->item(i-1)->setSelected(true);
      list_widget->scrollToItem(list_widget->item(i-1));
      break;
    }
  }
}

void ActionSearch::move_selection_down() {

  // Here we loop over all the items to find the currently selected one, and then select the one below it. We limit it
  // one entry before count() to efficiently ignore the item at the end (since the selection can't go below the very
  // bottom item).

  int lim = list_widget->count()-1;
  for (int i=0;i<lim;i++) {
    if (list_widget->item(i)->isSelected()) {
      list_widget->item(i+1)->setSelected(true);
      list_widget->scrollToItem(list_widget->item(i+1));
      break;
    }
  }
}

ActionSearchEntry::ActionSearchEntry(QWidget *parent) : QLineEdit(parent) {}

void ActionSearchEntry::keyPressEvent(QKeyEvent * event) {

  // Listen for up/down, otherwise pass the key event to the base class.

  switch (event->key()) {
  case Qt::Key_Up:
    emit moveSelectionUp();
    break;
  case Qt::Key_Down:
    emit moveSelectionDown();
    break;
  default:
    QLineEdit::keyPressEvent(event);
  }

}

ActionSearchList::ActionSearchList(QWidget *parent) : QListWidget(parent) {}

void ActionSearchList::mouseDoubleClickEvent(QMouseEvent *) {

  // Indiscriminately emit a signal on any double click
  emit dbl_click();

}
