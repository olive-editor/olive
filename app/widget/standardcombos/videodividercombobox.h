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

#ifndef VIDEODIVIDERCOMBOBOX_H
#define VIDEODIVIDERCOMBOBOX_H

#include <QComboBox>

#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

class VideoDividerComboBox : public QComboBox
{
  Q_OBJECT
public:
  VideoDividerComboBox(QWidget* parent = nullptr) :
    QComboBox(parent)
  {
    foreach (int d, VideoParams::kSupportedDividers) {
      QString name;

      if (d == 1) {
        name = tr("Full");
      } else {
        name = tr("1/%1").arg(d);
      }

      this->addItem(name, d);
    }
  }

  int GetDivider() const
  {
    return this->currentData().toInt();
  }

  void SetDivider(int d)
  {
    for (int i=0; i<this->count(); i++) {
      if (this->itemData(i).toInt() == d) {
        this->setCurrentIndex(i);
        break;
      }
    }
  }

};

OLIVE_NAMESPACE_EXIT

#endif // VIDEODIVIDERCOMBOBOX_H
