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

#ifndef PREFERENCESGENERALTAB_H
#define PREFERENCESGENERALTAB_H

#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>

#include "preferencestab.h"
#include "project/item/sequence/sequence.h"
#include "widget/slider/floatslider.h"

OLIVE_NAMESPACE_ENTER

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

public slots:
  void ResetDefaults(bool reset_all_tabs);

private:
  void SetValuesFromConfig(Config config);

  QComboBox* language_combobox_;

  QComboBox* autoscroll_method_;

  QCheckBox* rectified_waveforms_;

  FloatSlider* default_still_length_;

  /**
   * @brief A sequence we can feed to a SequenceDialog to change the defaults
   */
  Sequence default_sequence_;

};

OLIVE_NAMESPACE_EXIT

#endif // PREFERENCESGENERALTAB_H
