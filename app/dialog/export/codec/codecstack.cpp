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

#include "codecstack.h"

namespace olive {

#define super QStackedWidget

CodecStack::CodecStack(QWidget *parent)
  : super{parent}
{
  connect(this, &CodecStack::currentChanged, this, &CodecStack::OnChange);
}

void CodecStack::addWidget(QWidget *widget)
{
  super::addWidget(widget);

  OnChange(currentIndex());
}

void CodecStack::OnChange(int index)
{
  for (int i=0; i<count(); i++) {
    if (i == index) {
      widget(i)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    } else {
      widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }
    widget(i)->adjustSize();
  }
  adjustSize();
}

}
