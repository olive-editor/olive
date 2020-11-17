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

#ifndef CHANNELLAYOUTCOMBOBOX_H
#define CHANNELLAYOUTCOMBOBOX_H

#include <QComboBox>

#include "render/audioparams.h"

OLIVE_NAMESPACE_ENTER

class ChannelLayoutComboBox : public QComboBox
{
  Q_OBJECT
public:
  ChannelLayoutComboBox(QWidget* parent = nullptr) :
    QComboBox(parent)
  {
    foreach (const uint64_t& ch_layout, AudioParams::kSupportedChannelLayouts) {
      this->addItem(AudioParams::ChannelLayoutToString(ch_layout),
                    QVariant::fromValue(ch_layout));
    }
  }

  uint64_t GetChannelLayout() const
  {
    return this->currentData().toULongLong();
  }

  void SetChannelLayout(uint64_t ch)
  {
    for (int i=0; i<this->count(); i++) {
      if (this->itemData(i).toULongLong() == ch) {
        this->setCurrentIndex(i);
        break;
      }
    }
  }

};

OLIVE_NAMESPACE_EXIT

#endif // CHANNELLAYOUTCOMBOBOX_H
