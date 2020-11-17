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

#include "collapsebutton.h"

#include "ui/icons/icons.h"

OLIVE_NAMESPACE_ENTER

CollapseButton::CollapseButton(QWidget *parent) :
  QPushButton(parent)
{
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  setStyleSheet("border: none; background: none;");
  setCheckable(true);
  setChecked(true);
  setIconSize(QSize(fontMetrics().height()/2, fontMetrics().height()/2));

  connect(this, &CollapseButton::toggled, this, &CollapseButton::UpdateIcon);

  UpdateIcon(isChecked());
}

void CollapseButton::UpdateIcon(bool e)
{
  if (e) {
    setIcon(icon::TriDown);
  } else {
    setIcon(icon::TriRight);
  }
}

OLIVE_NAMESPACE_EXIT
