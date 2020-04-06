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

#ifndef PREFERENCESDISKTAB_H
#define PREFERENCESDISKTAB_H

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>

#include "preferencestab.h"
#include "widget/slider/floatslider.h"

OLIVE_NAMESPACE_ENTER

class PreferencesDiskTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesDiskTab();

  virtual void Accept() override;

private:
  QLineEdit* disk_cache_location_;

  FloatSlider* maximum_cache_slider_;

  FloatSlider* cache_ahead_slider_;

  FloatSlider* cache_behind_slider_;

  QCheckBox* clear_disk_cache_;

  QPushButton* clear_cache_btn_;

private slots:
  void DiskCacheLineEditChanged();

  void BrowseDiskCachePath();

  void ClearDiskCache();

};

OLIVE_NAMESPACE_EXIT

#endif // PREFERENCESDISKTAB_H
