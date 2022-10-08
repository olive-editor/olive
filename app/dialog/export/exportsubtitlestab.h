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

#ifndef EXPORTSUBTITLESTAB_H
#define EXPORTSUBTITLESTAB_H

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

#include "codec/exportformat.h"
#include "common/qtutils.h"
#include "dialog/export/exportformatcombobox.h"

namespace olive {

class ExportSubtitlesTab : public QWidget
{
  Q_OBJECT
public:
  ExportSubtitlesTab(QWidget *parent = nullptr);

  bool GetSidecarEnabled() const { return sidecar_checkbox_->isChecked(); }
  void SetSidecarEnabled(bool e) { sidecar_checkbox_->setEnabled(e); }

  ExportFormat::Format GetSidecarFormat() const { return sidecar_format_combobox_->GetFormat(); }
  void SetSidecarFormat(ExportFormat::Format f) { sidecar_format_combobox_->SetFormat(f); }

  int SetFormat(ExportFormat::Format format);

  ExportCodec::Codec GetSubtitleCodec()
  {
    return static_cast<ExportCodec::Codec>(codec_combobox_->currentData().toInt());
  }

  void SetSubtitleCodec(ExportCodec::Codec c)
  {
    QtUtils::SetComboBoxData(codec_combobox_, c);
  }

private:
  QCheckBox *sidecar_checkbox_;

  QLabel *sidecar_format_label_;
  ExportFormatComboBox *sidecar_format_combobox_;

  QComboBox *codec_combobox_;

};

}

#endif // EXPORTSUBTITLESTAB_H
