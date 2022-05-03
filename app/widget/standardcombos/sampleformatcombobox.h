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

#ifndef SAMPLEFORMATCOMBOBOX_H
#define SAMPLEFORMATCOMBOBOX_H

#include <QComboBox>

#include "render/audioparams.h"

namespace olive {

class SampleFormatComboBox : public QComboBox
{
  Q_OBJECT
public:
  SampleFormatComboBox(QWidget* parent = nullptr) :
    QComboBox(parent)
  {
    // Set up preview formats
    for (int i=0;i<AudioParams::kFormatCount;i++) {
      AudioParams::Format smp_fmt = static_cast<AudioParams::Format>(i);

      this->addItem(AudioParams::FormatToString(smp_fmt), smp_fmt);
    }
  }

  AudioParams::Format GetSampleFormat() const
  {
    return static_cast<AudioParams::Format>(this->currentData().toInt());
  }

  void SetSampleFormat(AudioParams::Format fmt)
  {
    for (int i=0; i<this->count(); i++) {
      if (this->itemData(i).toInt() == fmt) {
        this->setCurrentIndex(i);
        break;
      }
    }
  }

};

}

#endif // SAMPLEFORMATCOMBOBOX_H
