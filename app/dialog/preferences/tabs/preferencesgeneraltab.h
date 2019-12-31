#ifndef PREFERENCESGENERALTAB_H
#define PREFERENCESGENERALTAB_H

#include <QComboBox>
#include <QSpinBox>

#include "preferencestab.h"
#include "project/item/sequence/sequence.h"

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
  QComboBox* language_combobox_;

  QComboBox* autoscroll_method_;

  /**
   * @brief A sequence we can feed to a SequenceDialog to change the defaults
   */
  Sequence default_sequence_;


};

#endif // PREFERENCESGENERALTAB_H
