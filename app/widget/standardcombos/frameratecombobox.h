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

#ifndef FRAMERATECOMBOBOX_H
#define FRAMERATECOMBOBOX_H

#include <QComboBox>

#include "common/rational.h"
#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

class FrameRateComboBox : public QComboBox
{
  Q_OBJECT
public:
  FrameRateComboBox(QWidget* parent = nullptr) :
    QComboBox(parent)
  {
    foreach (const rational& fr, VideoParams::kSupportedFrameRates) {
      this->addItem(VideoParams::FrameRateToString(fr), QVariant::fromValue(fr));
    }
  }

  rational GetFrameRate() const
  {
    return this->currentData().value<rational>();
  }

  void SetFrameRate(const rational& r)
  {
    for (int i=0; i<this->count(); i++) {
      if (this->itemData(i).value<rational>() == r) {
        this->setCurrentIndex(i);
        break;
      }
    }
  }

};

OLIVE_NAMESPACE_EXIT

#endif // FRAMERATECOMBOBOX_H
