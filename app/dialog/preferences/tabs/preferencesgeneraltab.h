#ifndef PREFERENCESGENERALTAB_H
#define PREFERENCESGENERALTAB_H

#include <QComboBox>
#include <QSpinBox>

#include "preferencestab.h"

class PreferencesGeneralTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesGeneralTab();

  virtual void Accept() override;

private slots:
  /**
   * @brief Shows a NewSequenceDialog attached to default_sequence
   */
  void edit_default_sequence_settings();

private:
  /**
   * @brief UI widget for selecting the UI language
   */
  QComboBox* language_combobox;

  /**
   * @brief UI widget for selecting the resolution of the thumbnails to generate
   */
  QSpinBox* thumbnail_res_spinbox;

  /**
   * @brief UI widget for selecting the resolution of the waveforms to generate
   */
  QSpinBox* waveform_res_spinbox;

};

#endif // PREFERENCESGENERALTAB_H
