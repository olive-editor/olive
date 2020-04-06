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

#include <QAudioDeviceInfo>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

#include "audio/audiomanager.h"
#include "config/config.h"

OLIVE_NAMESPACE_ENTER

PreferencesAudioTab::PreferencesAudioTab() :
  has_devices_(false)
{
  QGridLayout* audio_tab_layout = new QGridLayout(this);
  audio_tab_layout->setMargin(0);

  int row = 0;

  // Audio -> Output Device
  audio_tab_layout->addWidget(new QLabel(tr("Output Device:")), row, 0);

  audio_output_devices = new QComboBox();
  audio_tab_layout->addWidget(audio_output_devices, row, 1);

  row++;

  // Audio -> Input Device
  audio_tab_layout->addWidget(new QLabel(tr("Input Device:")), row, 0);

  audio_input_devices = new QComboBox();
  audio_tab_layout->addWidget(audio_input_devices, row, 1);

  row++;

  // Audio -> Sample Rate

  audio_tab_layout->addWidget(new QLabel(tr("Sample Rate:")), row, 0);

  audio_sample_rate = new QComboBox();
  /*combobox_audio_sample_rates(audio_sample_rate);
  for (int i=0;i<audio_sample_rate->count();i++) {
    if (audio_sample_rate->itemData(i).toInt() == olive::config.audio_rate) {
      audio_sample_rate->setCurrentIndex(i);
      break;
    }
  }*/

  audio_tab_layout->addWidget(audio_sample_rate, row, 1);

  row++;

  // Audio -> Audio Recording
  audio_tab_layout->addWidget(new QLabel(tr("Audio Recording:"), this), row, 0);

  recordingComboBox = new QComboBox();
  recordingComboBox->addItem(tr("Mono"));
  recordingComboBox->addItem(tr("Stereo"));
//  recordingComboBox->setCurrentIndex(olive::config.recording_mode - 1);
  audio_tab_layout->addWidget(recordingComboBox, row, 1);

  row++;

  QPushButton* refresh_devices = new QPushButton(tr("Refresh Devices"));
  audio_tab_layout->addWidget(refresh_devices, row, 1);

  row++;

  RetrieveDeviceLists();

  connect(refresh_devices, SIGNAL(clicked(bool)), this, SLOT(RefreshDevices()));
  connect(AudioManager::instance(), SIGNAL(DeviceListReady()), this, SLOT(RetrieveDeviceLists()));
}

void PreferencesAudioTab::Accept()
{
  // If we don't have the device list, we can't set it
  if (!has_devices_) {
    return;
  }

  // Get device info
  QAudioDeviceInfo selected_output;
  QAudioDeviceInfo selected_input;

  QString selected_output_name;
  QString selected_input_name;

  // Index 0 is always the default device
  if (audio_output_devices->currentIndex() == 0) {
    selected_output = QAudioDeviceInfo::defaultOutputDevice();
  } else {
    selected_output = AudioManager::instance()->ListOutputDevices().at(audio_output_devices->currentData().toInt());
    selected_output_name = selected_output.deviceName();
  }

  // Index 0 is always the default device
  if (audio_input_devices->currentIndex() == 0) {
    selected_input = QAudioDeviceInfo::defaultInputDevice();
  } else {
    selected_input = AudioManager::instance()->ListInputDevices().at(audio_input_devices->currentData().toInt());
    selected_input_name = selected_input.deviceName();
  }

  // Save it in the global application preferences
  // FIXME: Qt documentation states that QAudioDeviceInfo::deviceName() is a "unique identifiers", which would make them
  //        ideal for saving in preferences, but in practice they don't actually appear to be unique.
  //        See: https://bugreports.qt.io/browse/QTBUG-16841
  if (Config::Current()["AudioOutput"] != selected_output_name) {
    Config::Current()["AudioOutput"] = selected_output_name;
    AudioManager::instance()->SetOutputDevice(selected_output);
  }

  if (Config::Current()["AudioInput"] != selected_input_name) {
    Config::Current()["AudioInput"] = selected_input_name;
    AudioManager::instance()->SetInputDevice(selected_input);
  }
}

void PreferencesAudioTab::RefreshDevices()
{
  AudioManager::instance()->RefreshDevices();

  RetrieveDeviceLists();
}

void PreferencesAudioTab::RetrieveDeviceLists()
{
  has_devices_ = false;

  audio_input_devices->clear();
  audio_output_devices->clear();

  if (AudioManager::instance()->IsRefreshing()) {
    audio_output_devices->addItem(tr("Please wait..."));
    audio_input_devices->addItem(tr("Please wait..."));
    audio_output_devices->setEnabled(false);
    audio_input_devices->setEnabled(false);
    return;
  }

  audio_output_devices->setEnabled(true);
  audio_input_devices->setEnabled(true);

  // list all available audio output devices
  bool found_preferred_device = false;
  audio_output_devices->addItem(tr("Default"), "");
  for (int i=0;i<AudioManager::instance()->ListOutputDevices().size();i++) {
    audio_output_devices->addItem(AudioManager::instance()->ListOutputDevices().at(i).deviceName(),
                                  i);
    if (!found_preferred_device
        && AudioManager::instance()->ListOutputDevices().at(i).deviceName() == Config::Current()["AudioOutput"]) {
      audio_output_devices->setCurrentIndex(audio_output_devices->count()-1);
      found_preferred_device = true;
    }
  }

  // list all available audio input devices
  found_preferred_device = false;
  audio_input_devices->addItem(tr("Default"), "");
  for (int i=0;i<AudioManager::instance()->ListInputDevices().size();i++) {
    audio_input_devices->addItem(AudioManager::instance()->ListInputDevices().at(i).deviceName(),
                                 i);
    if (!found_preferred_device
        && AudioManager::instance()->ListInputDevices().at(i).deviceName() == Config::Current()["AudioInput"]) {
      audio_input_devices->setCurrentIndex(audio_input_devices->count()-1);
      found_preferred_device = true;
    }
  }

  has_devices_ = true;
}

OLIVE_NAMESPACE_EXIT
