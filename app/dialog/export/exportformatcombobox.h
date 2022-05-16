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

#ifndef EXPORTFORMATCOMBOBOX_H
#define EXPORTFORMATCOMBOBOX_H

#include <QComboBox>

#include "codec/exportformat.h"

namespace olive {

class ExportFormatComboBox : public QComboBox
{
  Q_OBJECT
public:
  enum Mode {
    kShowAllFormats,
    kShowAudioOnly,
    kShowVideoOnly
  };

  ExportFormatComboBox(Mode mode, QWidget *parent = nullptr);
  ExportFormatComboBox(QWidget *parent = nullptr) :
    ExportFormatComboBox(kShowAllFormats, parent)
  {}

  ExportFormat::Format GetFormat() const
  {
    return static_cast<ExportFormat::Format>(currentData().toInt());
  }

signals:
  void FormatChanged(ExportFormat::Format fmt);

public slots:
  void SetFormat(ExportFormat::Format fmt);

private slots:
  void HandleIndexChange(int index);

};

}

#endif // EXPORTFORMATCOMBOBOX_H
