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

#ifndef CHANNELLAYOUTCOMBOBOX_H
#define CHANNELLAYOUTCOMBOBOX_H

#include <QComboBox>

#include "render/audioparams.h"
#include "ui/humanstrings.h"

namespace olive {

class ChannelLayoutComboBox : public QComboBox
{
  Q_OBJECT
public:
  ChannelLayoutComboBox(QWidget* parent = nullptr) :
    QComboBox(parent)
  {
    foreach (const AudioChannelLayout& ch_layout, AudioParams::kSupportedChannelLayouts) {
      this->addItem(ch_layout.toHumanString(), QVariant::fromValue(ch_layout));
    }
  }

  AudioChannelLayout GetChannelLayout() const
  {
    return this->currentData().value<AudioChannelLayout>();
  }

  void SetChannelLayout(const AudioChannelLayout &ch)
  {
    for (int i=0; i<this->count(); i++) {
      if (this->itemData(i).value<AudioChannelLayout>() == ch) {
        this->setCurrentIndex(i);
        break;
      }
    }
  }

};

}

#endif // CHANNELLAYOUTCOMBOBOX_H
