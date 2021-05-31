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

#ifndef PREFERENCESAUDIOTAB_H
#define PREFERENCESAUDIOTAB_H

#include <QAudioDeviceInfo>
#include <QComboBox>
#include <QPushButton>

#include "dialog/configbase/configdialogbase.h"

namespace olive {

class PreferencesAudioTab : public ConfigDialogBaseTab
{
  Q_OBJECT
public:
  PreferencesAudioTab();

  virtual void Accept(MultiUndoCommand* command) override;

private:
  QComboBox* audio_backend_combobox_;

  /**
   * @brief UI widget for selecting the output audio device
   */
  QComboBox* audio_output_devices_;

  /**
   * @brief UI widget for selecting the input audio device
   */
  QComboBox* audio_input_devices_;

  /**
   * @brief UI widget for editing the recording channels
   */
  QComboBox* recording_combobox_;

  /**
   * @brief Button that triggers a refresh of the available audio devices
   */
  QPushButton* refresh_devices_btn_;

private slots:
  void RefreshDevices();

  void RetrieveOutputList();

  void RetrieveInputList();

private:
  void RetrieveDeviceLists();

  void UpdateRefreshButtonEnabled();

  static void PopulateComboBox(QComboBox* cb, bool still_refreshing, const QList<QAudioDeviceInfo>& list, const QString &preferred);

};

}

#endif // PREFERENCESAUDIOTAB_H
