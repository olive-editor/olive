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
    QComboBox(parent),
    attempt_to_restore_format_(true)
  {
  }

  void SetAttemptToRestoreFormat(bool e) { attempt_to_restore_format_ = e; }

  void SetAvailableFormats(const std::vector<AudioParams::Format> &formats)
  {
    AudioParams::Format tmp = AudioParams::kFormatInvalid;

    if (attempt_to_restore_format_) {
      tmp = GetSampleFormat();
    }

    clear();
    foreach (const AudioParams::Format &of, formats) {
      AddFormatItem(of);
    }

    if (attempt_to_restore_format_) {
      SetSampleFormat(tmp);
    }
  }

  void SetPackedFormats()
  {
    AudioParams::Format tmp = AudioParams::kFormatInvalid;

    if (attempt_to_restore_format_) {
      tmp = GetSampleFormat();
    }

    clear();
    for (int i=AudioParams::kPackedStart; i<AudioParams::kPackedEnd; i++) {
      AddFormatItem(static_cast<AudioParams::Format>(i));
    }

    if (attempt_to_restore_format_) {
      SetSampleFormat(tmp);
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

private:
  void AddFormatItem(AudioParams::Format f)
  {
    this->addItem(AudioParams::FormatToString(f), f);
  }

  bool attempt_to_restore_format_;

};

}

#endif // SAMPLEFORMATCOMBOBOX_H
