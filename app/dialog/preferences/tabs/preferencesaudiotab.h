#ifndef PREFERENCESAUDIOTAB_H
#define PREFERENCESAUDIOTAB_H

#include <QComboBox>

#include "preferencestab.h"

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
  QComboBox* audio_output_devices;

  /**
   * @brief UI widget for selecting the input audio device
   */
  QComboBox* audio_input_devices;

  /**
   * @brief UI widget for selecting the audio sampling rates
   */
  QComboBox* audio_sample_rate;

  /**
   * @brief UI widget for editing the recording channels
   */
  QComboBox* recordingComboBox;

private slots:
  void RefreshDevices();

  void RetrieveDeviceLists();

private:
  bool has_devices_;

};

#endif // PREFERENCESAUDIOTAB_H
