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

#ifndef PREFERENCESAUDIOTAB_H
#define PREFERENCESAUDIOTAB_H

#include <QAudioDeviceInfo>
#include <QComboBox>

#include "preferencestab.h"

OLIVE_NAMESPACE_ENTER

class PreferencesAudioTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesAudioTab();

  virtual void Accept() override;

private:
  /**
   * @brief UI widget for selecting the output audio device
   */
  QComboBox* audio_output_devices_;

  /**
   * @brief UI widget for selecting the input audio device
   */
  QComboBox* audio_input_devices_;

  /**
   * @brief UI widget for selecting the audio sampling rates
   */
  QComboBox* audio_sample_rate_;

  /**
   * @brief UI widget for editing the recording channels
   */
  QComboBox* recording_combobox_;

private slots:
  void RefreshDevices();

  void RetrieveOutputList();

  void RetrieveInputList();

private:
  void RetrieveDeviceLists();

  static void PopulateComboBox(QComboBox* cb, bool still_refreshing, const QList<QAudioDeviceInfo>& list, const QString &preferred);

};

OLIVE_NAMESPACE_EXIT

#endif // PREFERENCESAUDIOTAB_H
