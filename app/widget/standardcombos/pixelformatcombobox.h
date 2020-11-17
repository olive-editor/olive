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

#ifndef PIXELFORMATCOMBOBOX_H
#define PIXELFORMATCOMBOBOX_H

#include <QComboBox>

#include "render/videoparams.h"

namespace olive {

class PixelFormatComboBox : public QComboBox
{
  Q_OBJECT
public:
  PixelFormatComboBox(bool float_only, QWidget* parent = nullptr) :
    QComboBox(parent)
  {
    // Set up preview formats
    for (int i=0;i<VideoParams::kFormatCount;i++) {
      VideoParams::Format pix_fmt = static_cast<VideoParams::Format>(i);

      if (!float_only || VideoParams::FormatIsFloat(pix_fmt)) {
        this->addItem(VideoParams::GetFormatName(pix_fmt), pix_fmt);
      }
    }
  }

  VideoParams::Format GetPixelFormat() const
  {
    return static_cast<VideoParams::Format>(this->currentData().toInt());
  }

  void SetPixelFormat(VideoParams::Format fmt)
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

#endif // PIXELFORMATCOMBOBOX_H
