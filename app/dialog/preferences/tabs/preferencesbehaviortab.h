/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2025 mikesolar

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

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QVBoxLayout>

#include "dialog/configbase/configdialogbase.h"

namespace olive
{

class PreferencesBehaviorTab : public ConfigDialogBaseTab {
	Q_OBJECT
public:
	enum Category {
		kCategoryTimeline,
		kCategoryPlayback,
		kCategoryProject,
		kCategoryNodes,
		kCategoryRendering
	};

	PreferencesBehaviorTab(Category category);

	virtual void Accept(MultiUndoCommand *command) override;

	static QString BehaviorPrefTr(const char *text)
	{
		return QCoreApplication::translate("olive::PreferencesBehaviorTab",
										   text);
	}

private:
	struct Item {
		QString text;
		QString config_key;
		QString tooltip = QString();
	};

	void AddItems(const QVector<Item> &items);
	QCheckBox *AddItem(const QString &text, const QString &config_key,
					   const QString &tooltip = QString());

	QMap<QCheckBox *, QString> config_map_;

	Category category_;

	QComboBox *graphics_backend_combobox_;
};

}

#endif // PREFERENCESBEHAVIORTAB_H
