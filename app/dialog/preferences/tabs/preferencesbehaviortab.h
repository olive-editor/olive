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

#ifndef PREFERENCESBEHAVIORTAB_H
#define PREFERENCESBEHAVIORTAB_H

#include <QTreeWidget>

#include "preferencestab.h"

OLIVE_NAMESPACE_ENTER

class PreferencesBehaviorTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesBehaviorTab();

  virtual void Accept() override;

private:
  QTreeWidgetItem *AddParent(const QString& text, const QString &tooltip, QTreeWidgetItem *parent = nullptr);
  QTreeWidgetItem *AddParent(const QString& text, QTreeWidgetItem *parent = nullptr);

  QTreeWidgetItem *AddItem(const QString& text, const QString& config_key, const QString &tooltip, QTreeWidgetItem *parent );
  QTreeWidgetItem *AddItem(const QString& text, const QString& config_key, QTreeWidgetItem *parent);

  QMap<QTreeWidgetItem*, QString> config_map_;

  QTreeWidget* behavior_tree_;

};

OLIVE_NAMESPACE_EXIT

#endif // PREFERENCESBEHAVIORTAB_H
