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

#ifndef SAMPLEFORMATCOMBOBOX_H
#define SAMPLEFORMATCOMBOBOX_H

#include <QComboBox>

#include "ui/humanstrings.h"

namespace olive {

class SampleFormatComboBox : public QComboBox
{
  Q_OBJECT
public:
  SampleFormatComboBox(QWidget* parent = nullptr) :
    QComboBox(parent),
    attempt_to_restore_format_(true)
  {
  }

  void SetAttemptToRestoreFormat(bool e) { attempt_to_restore_format_ = e; }

  void SetAvailableFormats(const std::vector<SampleFormat> &formats)
  {
    SampleFormat tmp = SampleFormat::INVALID;

    if (attempt_to_restore_format_) {
      tmp = GetSampleFormat();
    }

    clear();
    foreach (const SampleFormat &of, formats) {
      AddFormatItem(of);
    }

    if (attempt_to_restore_format_) {
      SetSampleFormat(tmp);
    }
  }

  void SetPackedFormats()
  {
    SampleFormat tmp = SampleFormat::INVALID;

    if (attempt_to_restore_format_) {
      tmp = GetSampleFormat();
    }

    clear();
    for (int i=SampleFormat::PACKED_START; i<SampleFormat::PACKED_END; i++) {
      AddFormatItem(static_cast<SampleFormat::Format>(i));
    }

    if (attempt_to_restore_format_) {
      SetSampleFormat(tmp);
    }
  }

  SampleFormat GetSampleFormat() const
  {
    return static_cast<SampleFormat::Format>(this->currentData().toInt());
  }

  void SetSampleFormat(SampleFormat fmt)
  {
    for (int i=0; i<this->count(); i++) {
      if (this->itemData(i).toInt() == fmt) {
        this->setCurrentIndex(i);
        break;
      }
    }
  }

private:
  void AddFormatItem(SampleFormat f)
  {
    this->addItem(HumanStrings::FormatToString(f), static_cast<SampleFormat::Format>(f));
  }

  bool attempt_to_restore_format_;

};

}

#endif // SAMPLEFORMATCOMBOBOX_H
