#include "preferencesaudiotab.h"

#include <QAudioDeviceInfo>
#include <QGridLayout>
#include <QLabel>

#include "config/config.h"

PreferencesAudioTab::PreferencesAudioTab()
{
  QGridLayout* audio_tab_layout = new QGridLayout(this);

  int row = 0;

  // Audio -> Output Device

  audio_tab_layout->addWidget(new QLabel(tr("Output Device:")), row, 0);

  audio_output_devices = new QComboBox();
  audio_output_devices->addItem(tr("Default"), "");

  // list all available audio output devices
  QList<QAudioDeviceInfo> devs = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
  bool found_preferred_device = false;
  for (int i=0;i<devs.size();i++) {
    audio_output_devices->addItem(devs.at(i).deviceName(), devs.at(i).deviceName());
    if (!found_preferred_device
        && devs.at(i).deviceName() == Config::Current()["AudioOutput"]) {
      audio_output_devices->setCurrentIndex(audio_output_devices->count()-1);
      found_preferred_device = true;
    }
  }

  audio_tab_layout->addWidget(audio_output_devices, row, 1);

  row++;

  // Audio -> Input Device

  audio_tab_layout->addWidget(new QLabel(tr("Input Device:")), row, 0);

  audio_input_devices = new QComboBox();
  audio_input_devices->addItem(tr("Default"), "");

  // list all available audio input devices
  devs = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
  found_preferred_device = false;
  for (int i=0;i<devs.size();i++) {
    audio_input_devices->addItem(devs.at(i).deviceName(), devs.at(i).deviceName());
    if (!found_preferred_device
        && devs.at(i).deviceName() == Config::Current()["AudioInput"]) {
      audio_input_devices->setCurrentIndex(audio_input_devices->count()-1);
      found_preferred_device = true;
    }
  }

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
}

void PreferencesAudioTab::Accept()
{

}
