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

#include "fontcombobox.h"

#include <QFontDatabase>

FontCombobox::FontCombobox(QWidget* parent) : QComboBox(parent) {
  addItems(QFontDatabase().families());

  value = currentText();

  connect(this, SIGNAL(currentTextChanged(QString)), this, SLOT(updateInternals()));
}

const QString& FontCombobox::getPreviousValue() {
  return previousValue;
}

void FontCombobox::updateInternals() {
  previousValue = value;
  value = currentText();
}
