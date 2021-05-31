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

#ifndef PREFERENCESGENERALTAB_H
#define PREFERENCESGENERALTAB_H

#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>

#include "dialog/configbase/configdialogbase.h"
#include "node/project/sequence/sequence.h"
#include "widget/slider/rationalslider.h"
#include "widget/slider/integerslider.h"

namespace olive {

class PreferencesGeneralTab : public ConfigDialogBaseTab
{
  Q_OBJECT
public:
  PreferencesGeneralTab();

  virtual void Accept(MultiUndoCommand* command) override;

private:
  void AddLanguage(const QString& locale_name);

  QComboBox* language_combobox_;

  QComboBox* autoscroll_method_;

  QCheckBox* rectified_waveforms_;

  RationalSlider* default_still_length_;

  QCheckBox* autorecovery_enabled_;

  IntegerSlider* autorecovery_interval_;

  IntegerSlider* autorecovery_maximum_;

};

}

#endif // PREFERENCESGENERALTAB_H
