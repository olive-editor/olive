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

#include "preferencesaudiotab.h"

#include <QGridLayout>
#include <QLabel>

#include "audio/audiomanager.h"
#include "config/config.h"

OLIVE_NAMESPACE_ENTER

PreferencesAudioTab::PreferencesAudioTab()
{
  QGridLayout* audio_tab_layout = new QGridLayout(this);
  audio_tab_layout->setMargin(0);

  int row = 0;

  // Audio -> Output Device
  audio_tab_layout->addWidget(new QLabel(tr("Output Device:")), row, 0);

  audio_output_devices_ = new QComboBox();
  audio_tab_layout->addWidget(audio_output_devices_, row, 1);

  row++;

  // Audio -> Input Device
  audio_tab_layout->addWidget(new QLabel(tr("Input Device:")), row, 0);

  audio_input_devices_ = new QComboBox();
  audio_tab_layout->addWidget(audio_input_devices_, row, 1);

  row++;

  // Audio -> Sample Rate

  audio_tab_layout->addWidget(new QLabel(tr("Sample Rate:")), row, 0);

  audio_sample_rate_ = new QComboBox();
  /*combobox_audio_sample_rates(audio_sample_rate);
  for (int i=0;i<audio_sample_rate->count();i++) {
    if (audio_sample_rate->itemData(i).toInt() == olive::config.audio_rate) {
      audio_sample_rate->setCurrentIndex(i);
      break;
    }
  }*/

  audio_tab_layout->addWidget(audio_sample_rate_, row, 1);

  row++;

  // Audio -> Audio Recording
  audio_tab_layout->addWidget(new QLabel(tr("Audio Recording:"), this), row, 0);

  recording_combobox_ = new QComboBox();
  recording_combobox_->addItem(tr("Mono"));
  recording_combobox_->addItem(tr("Stereo"));
//  recordingComboBox->setCurrentIndex(olive::config.recording_mode - 1);
  audio_tab_layout->addWidget(recording_combobox_, row, 1);

  row++;

  refresh_devices_btn_ = new QPushButton(tr("Refresh Devices"));
  audio_tab_layout->addWidget(refresh_devices_btn_, row, 1);

  row++;

  RetrieveDeviceLists();

  connect(refresh_devices_btn_, &QPushButton::clicked, this, &PreferencesAudioTab::RefreshDevices);
  connect(AudioManager::instance(), &AudioManager::OutputListReady, this, &PreferencesAudioTab::RetrieveOutputList);
  connect(AudioManager::instance(), &AudioManager::InputListReady, this, &PreferencesAudioTab::RetrieveInputList);

  SetValuesFromConfig(Config::Current());
}

void PreferencesAudioTab::Accept()
{
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
      Config::Current()["AudioOutput"] = QVariant::fromValue(selected_output_name);
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
      Config::Current()["AudioInput"] = QVariant::fromValue(selected_input_name);
      AudioManager::instance()->SetInputDevice(selected_input);
    }
  }

  Config::Current()["AudioRecording"] = QVariant::fromValue(recording_combobox_->currentText());
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

void PreferencesAudioTab::SetValuesFromConfig(Config config)
{
  if (audio_output_devices_->isEnabled()) {
    if (config["AudioOutput"] == QString("")) {
      audio_output_devices_->setCurrentIndex(audio_output_devices_->findText("Default"));
    } else {
      audio_output_devices_->setCurrentIndex(audio_output_devices_->findText(config["AudioOutput"].toString()));
    }
  }

  if (audio_input_devices_->isEnabled()) {
    if (config["AudioInput"] == QString("")) {
      audio_input_devices_->setCurrentIndex(audio_input_devices_->findText("Default"));
    } else {
      audio_input_devices_->setCurrentIndex(audio_input_devices_->findText(config["AudioInput"].toString()));
    }
  }

  recording_combobox_->setCurrentIndex(recording_combobox_->findText(config["AudioRecording"].toString()));
}

void PreferencesAudioTab::ResetDefaults(bool reset_all_tabs) {
  bool confirm_reset = true;
  if (!reset_all_tabs) {
    confirm_reset = QMessageBox::question(this, tr("Confirm Reset Audio Settings"),
                                          tr("Are you sure you wish to reset all Audio settings?"),
                                          QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
  }

  if (confirm_reset) {
      Config default_config;
      SetValuesFromConfig(default_config);
  }
}



OLIVE_NAMESPACE_EXIT
