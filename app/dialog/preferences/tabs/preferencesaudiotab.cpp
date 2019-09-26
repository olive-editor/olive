#include "preferencesaudiotab.h"

#include <QAudioDeviceInfo>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

#include "audio/audiomanager.h"
#include "config/config.h"

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
                                  AudioManager::instance()->ListOutputDevices().at(i).deviceName());
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
                                 AudioManager::instance()->ListInputDevices().at(i).deviceName());
    if (!found_preferred_device
        && AudioManager::instance()->ListInputDevices().at(i).deviceName() == Config::Current()["AudioInput"]) {
      audio_input_devices->setCurrentIndex(audio_input_devices->count()-1);
      found_preferred_device = true;
    }
  }

  has_devices_ = true;
}
