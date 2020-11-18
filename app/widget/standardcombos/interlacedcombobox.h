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

#ifndef INTERLACEDCOMBOBOX_H
#define INTERLACEDCOMBOBOX_H

#include <QComboBox>

#include "render/videoparams.h"

namespace olive {

class InterlacedComboBox : public QComboBox
{
  Q_OBJECT
public:
  InterlacedComboBox(QWidget* parent = nullptr) :
    QComboBox(parent)
  {
    // These must match the Interlacing enum in VideoParams
    this->addItem(tr("None (Progressive)"));
    this->addItem(tr("Top-Field First"));
    this->addItem(tr("Bottom-Field First"));
  }

  VideoParams::Interlacing GetInterlaceMode() const
  {
    return static_cast<VideoParams::Interlacing>(this->currentIndex());
  }

  void SetInterlaceMode(VideoParams::Interlacing mode)
  {
    this->setCurrentIndex(mode);
  }

};

}

#endif // INTERLACEDCOMBOBOX_H
