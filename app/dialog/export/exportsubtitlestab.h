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

#ifndef EXPORTSUBTITLESTAB_H
#define EXPORTSUBTITLESTAB_H

#include <QComboBox>

#include "codec/exportformat.h"
#include "render/subtitleparams.h"

namespace olive {

class ExportSubtitlesTab : public QWidget
{
  Q_OBJECT
public:
  ExportSubtitlesTab(QWidget *parent = nullptr);

  int SetFormat(ExportFormat::Format format);

  ExportCodec::Codec GetSubtitleCodec()
  {
    return static_cast<ExportCodec::Codec>(codec_combobox_->currentData().toInt());
  }

private:
  QComboBox *codec_combobox_;

};

}

#endif // EXPORTSUBTITLESTAB_H
