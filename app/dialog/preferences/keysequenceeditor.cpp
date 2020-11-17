/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "keysequenceeditor.h"

#include <QAction>

OLIVE_NAMESPACE_ENTER

KeySequenceEditor::KeySequenceEditor(QWidget* parent, QAction* a)
  : QKeySequenceEdit(parent), action(a) {
  setKeySequence(action->shortcut());
}

void KeySequenceEditor::set_action_shortcut() {
  action->setShortcut(keySequence());
}

void KeySequenceEditor::reset_to_default() {
  setKeySequence(action->property("keydefault").toString());
}

QString KeySequenceEditor::action_name() {
  return action->property("id").toString();
}

QString KeySequenceEditor::export_shortcut() {
  QString ks = keySequence().toString();
  if (ks != action->property("keydefault")) {
    return action->property("id").toString() + "\t" + ks;
  }
  return nullptr;
}

OLIVE_NAMESPACE_EXIT
