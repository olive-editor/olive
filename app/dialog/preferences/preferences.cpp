/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "preferences.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QSplitter>
#include <QVBoxLayout>

#include "config/config.h"
#include "tabs/preferencesgeneraltab.h"
#include "tabs/preferencesbehaviortab.h"
#include "tabs/preferencesappearancetab.h"
#include "tabs/preferencesdisktab.h"
#include "tabs/preferencesaudiotab.h"
#include "tabs/preferenceskeyboardtab.h"
#include "tabs/preferencesedittab.h"
#include "window/mainwindow/mainwindow.h"


namespace olive {

PreferencesDialog::PreferencesDialog(MainWindow *main_window) :
  ConfigDialogBase(main_window)
{
  setWindowTitle(tr("Preferences"));

  AddTab(new PreferencesGeneralTab(), tr("General"));
  AddTab(new PreferencesAppearanceTab(), tr("Appearance"));
  AddTab(new PreferencesBehaviorTab(), tr("Behavior"));
  AddTab(new PreferencesDiskTab(), tr("Disk"));
  AddTab(new PreferencesAudioTab(), tr("Audio"));
  AddTab(new PreferencesEditTab(), tr("Text Editor"));
  AddTab(new PreferencesKeyboardTab(main_window), tr("Keyboard"));
}

void PreferencesDialog::AcceptEvent()
{
  Config::Save();
}

}
