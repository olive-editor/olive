/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef SAMPLERATECOMBOBOX_H
#define SAMPLERATECOMBOBOX_H

#include <QComboBox>

#include "render/audioparams.h"

OLIVE_NAMESPACE_ENTER

class SampleRateComboBox : public QComboBox
{
  Q_OBJECT
public:
  SampleRateComboBox(QWidget* parent = nullptr) :
    QComboBox(parent)
  {
    foreach (int sr, AudioParams::kSupportedSampleRates) {
      this->addItem(AudioParams::SampleRateToString(sr), sr);
    }
  }

  int GetSampleRate() const
  {
    return this->currentData().toInt();
  }

  void SetSampleRate(int rate)
  {
    for (int i=0; i<this->count(); i++) {
      if (this->itemData(i).toInt() == rate) {
        this->setCurrentIndex(i);
        break;
      }
    }
  }

};

OLIVE_NAMESPACE_EXIT

#endif // SAMPLERATECOMBOBOX_H
