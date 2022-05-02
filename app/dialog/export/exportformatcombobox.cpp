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

#include "exportformatcombobox.h"

namespace olive {

ExportFormatComboBox::ExportFormatComboBox(Mode mode, QWidget *parent) :
  QComboBox(parent)
{
  // Populate combobox formats
  for (int i=0; i<ExportFormat::kFormatCount; i++) {
    ExportFormat::Format f = static_cast<ExportFormat::Format>(i);

    switch (mode) {
    case kShowAllFormats:
      break;
    case kShowAudioOnly:
      if (!ExportFormat::GetVideoCodecs(f).isEmpty()) {
        continue;
      }
      break;
    case kShowVideoOnly:
      if (!ExportFormat::GetAudioCodecs(f).isEmpty()) {
        continue;
      }
      break;
    }

    QString format_name = ExportFormat::GetName(f);

    bool inserted = false;

    // Sort formats alphabetically
    for (int j=0; j<count(); j++) {
      if (itemText(j) > format_name) {
        insertItem(j, format_name, i);
        inserted = true;
        break;
      }
    }

    if (!inserted) {
      addItem(format_name, i);
    }
  }

  connect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportFormatComboBox::HandleIndexChange);
}

void ExportFormatComboBox::SetFormat(ExportFormat::Format fmt)
{
  for (int i=0; i<count(); i++) {
    if (itemData(i).toInt() == fmt) {
      setCurrentIndex(i);
      break;
    }
  }
}

void ExportFormatComboBox::HandleIndexChange(int index)
{
  emit FormatChanged(static_cast<ExportFormat::Format>(itemData(index).toInt()));
}

}
