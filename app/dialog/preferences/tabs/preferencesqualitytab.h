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

#ifndef PREFERENCESQUALITYTAB_H
#define PREFERENCESQUALITYTAB_H

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QStackedWidget>

#include "render/pixelformat.h"
#include "preferencestab.h"

OLIVE_NAMESPACE_ENTER

class PreferencesQualityGroup : public QGroupBox
{
  Q_OBJECT
public:
  PreferencesQualityGroup(const QString& title, QWidget* parent = nullptr);

  void SetBitDepth(PixelFormat::Format f);

  QComboBox* bit_depth_combobox();

  QComboBox* ocio_method();

private:
  QComboBox* bit_depth_combobox_;

  QComboBox* ocio_method_;

};

class PreferencesQualityTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesQualityTab();

  virtual void Accept() override;

private:
  QStackedWidget* quality_stack_;

  PreferencesQualityGroup* offline_group_;

  PreferencesQualityGroup* online_group_;

};

OLIVE_NAMESPACE_EXIT

#endif // PREFERENCESQUALITYTAB_H
