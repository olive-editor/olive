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

#include "focusablelineedit.h"

#include <QKeyEvent>

namespace olive {

FocusableLineEdit::FocusableLineEdit(QWidget *parent) :
  QLineEdit(parent)
{

}

void FocusableLineEdit::keyPressEvent(QKeyEvent *e)
{
  switch (e->key()) {
  case Qt::Key_Return:
  case Qt::Key_Enter:
    emit Confirmed();
    break;
  case Qt::Key_Escape:
    emit Cancelled();
    break;
  default:
    QLineEdit::keyPressEvent(e);
  }
}

void FocusableLineEdit::focusOutEvent(QFocusEvent *e)
{
  QLineEdit::focusOutEvent(e);

  emit Confirmed();
}

}
