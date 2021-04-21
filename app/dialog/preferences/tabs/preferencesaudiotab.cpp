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

#include "preferencesaudiotab.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

#include "audio/audiomanager.h"
#include "config/config.h"

namespace olive {

PreferencesAudioTab::PreferencesAudioTab()
{
  QVBoxLayout* audio_tab_layout = new QVBoxLayout(this);

  {
    // Backend Layout
    QGridLayout* main_layout = new QGridLayout();
    main_layout->setMargin(0);

    int row = 0;

    main_layout->addWidget(new QLabel(tr("Backend:")), row, 0);

    audio_backend_combobox_ = new QComboBox();
    for (int i=0; i<AudioManager::kAudioBackendCount; i++) {
      audio_backend_combobox_->addItem(AudioManager::GetAudioBackendName(static_cast<AudioManager::Backend>(i)));
    }
    main_layout->addWidget(audio_backend_combobox_, row, 1);

    audio_tab_layout->addLayout(main_layout);
  }

  {
    // Qt-Backend Layout
    QGroupBox* qt_groupbox = new QGroupBox();
    audio_tab_layout->addWidget(qt_groupbox);

    QVBoxLayout* qt_layout = new QVBoxLayout(qt_groupbox);

    int row = 0;

    {
      // Output Group
      QGroupBox* qt_output_group = new QGroupBox();
      qt_output_group->setTitle(tr("Output"));
      qt_layout->addWidget(qt_output_group);

      QGridLayout* qt_output_layout = new QGridLayout(qt_output_group);

      qt_output_layout->addWidget(new QLabel(tr("Device:")), row, 0);

      audio_output_devices_ = new QComboBox();
      qt_output_layout->addWidget(audio_output_devices_, row, 1);
    }

    row = 0;

    {
      QGroupBox* qt_input_group = new QGroupBox();
      qt_input_group->setTitle(tr("Input"));
      qt_layout->addWidget(qt_input_group);

      QGridLayout* qt_input_layout = new QGridLayout(qt_input_group);

      qt_input_layout->addWidget(new QLabel(tr("Device:")), row, 0);

      audio_input_devices_ = new QComboBox();
      qt_input_layout->addWidget(audio_input_devices_, row, 1);

      row++;

      qt_input_layout->addWidget(new QLabel(tr("Recording Mode:"), this), row, 0);

      recording_combobox_ = new QComboBox();
      recording_combobox_->addItem(tr("Mono"));
      recording_combobox_->addItem(tr("Stereo"));
      qt_input_layout->addWidget(recording_combobox_, row, 1);
    }

    QHBoxLayout* qt_refresh_layout = new QHBoxLayout();
    qt_layout->addLayout(qt_refresh_layout);
    qt_refresh_layout->addStretch();

    refresh_devices_btn_ = new QPushButton(tr("Refresh Devices"));
    qt_refresh_layout->addWidget(refresh_devices_btn_);

    RetrieveDeviceLists();

    connect(refresh_devices_btn_, &QPushButton::clicked, this, &PreferencesAudioTab::RefreshDevices);
    connect(AudioManager::instance(), &AudioManager::OutputListReady, this, &PreferencesAudioTab::RetrieveOutputList);
    connect(AudioManager::instance(), &AudioManager::InputListReady, this, &PreferencesAudioTab::RetrieveInputList);
  }

  audio_tab_layout->addStretch();
}

void PreferencesAudioTab::Accept(MultiUndoCommand *command)
{
  Q_UNUSED(command)

  // FIXME: Qt documentation states that QAudioDeviceInfo::deviceName() is a "unique identifiers", which would make them
  //        ideal for saving in preferences, but in practice they don't actually appear to be unique.
  //        See: https://bugreports.qt.io/browse/QTBUG-16841

  // If we don't have the device list, we can't set it
  if (audio_output_devices_->isEnabled()) {
    // Get device info
    QAudioDeviceInfo selected_output;
    QString selected_output_name;

    // Index 0 is always the default device
    if (audio_output_devices_->currentIndex() == 0) {
      selected_output = QAudioDeviceInfo::defaultOutputDevice();
    } else {
      selected_output = AudioManager::instance()->ListOutputDevices().at(audio_output_devices_->currentData().toInt());
      selected_output_name = selected_output.deviceName();
    }

    // Save it in the global application preferences
    if (Config::Current()["AudioOutput"] != selected_output_name) {
      Config::Current()["AudioOutput"] = selected_output_name;
      AudioManager::instance()->SetOutputDevice(selected_output);
    }
  }

  if (audio_input_devices_->isEnabled()) {
    QAudioDeviceInfo selected_input;
    QString selected_input_name;

    // Index 0 is always the default device
    if (audio_input_devices_->currentIndex() == 0) {
      selected_input = QAudioDeviceInfo::defaultInputDevice();
    } else {
      selected_input = AudioManager::instance()->ListInputDevices().at(audio_input_devices_->currentData().toInt());
      selected_input_name = selected_input.deviceName();
    }

    if (Config::Current()["AudioInput"] != selected_input_name) {
      Config::Current()["AudioInput"] = selected_input_name;
      AudioManager::instance()->SetInputDevice(selected_input);
    }
  }
}

void PreferencesAudioTab::RefreshDevices()
{
  AudioManager::instance()->RefreshDevices();

  RetrieveDeviceLists();
}

void PreferencesAudioTab::RetrieveOutputList()
{
  PopulateComboBox(audio_output_devices_,
                   AudioManager::instance()->IsRefreshingOutputs(),
                   AudioManager::instance()->ListOutputDevices(),
                   Config::Current()["AudioOutput"].toString());

  UpdateRefreshButtonEnabled();
}

void PreferencesAudioTab::RetrieveInputList()
{
  PopulateComboBox(audio_input_devices_,
                   AudioManager::instance()->IsRefreshingInputs(),
                   AudioManager::instance()->ListInputDevices(),
                   Config::Current()["AudioInput"].toString());

  UpdateRefreshButtonEnabled();
}

void PreferencesAudioTab::RetrieveDeviceLists()
{
  RetrieveOutputList();
  RetrieveInputList();
}

void PreferencesAudioTab::UpdateRefreshButtonEnabled()
{
  refresh_devices_btn_->setEnabled(audio_output_devices_->isEnabled()
                                   && audio_input_devices_->isEnabled());
}

void PreferencesAudioTab::PopulateComboBox(QComboBox *cb, bool still_refreshing, const QList<QAudioDeviceInfo> &list, const QString& preferred)
{
  cb->clear();

  cb->setEnabled(!still_refreshing);

  if (still_refreshing) {
    cb->addItem(tr("Please wait..."));
  } else {
    bool found_preferred_device = false;

    // Add null default item
    cb->addItem(tr("Default"), QVariant());

    // For each entry, add it to the combobox
    for (int i=0;i<list.size();i++) {

      cb->addItem(list.at(i).deviceName(), i);

      if (!found_preferred_device
          && list.at(i).deviceName() == preferred) {
        cb->setCurrentIndex(cb->count()-1);
        found_preferred_device = true;
      }

    }

  }
}

}
