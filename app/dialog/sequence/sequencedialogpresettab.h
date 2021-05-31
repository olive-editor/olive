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

#ifndef SEQUENCEDIALOGPRESETTAB_H
#define SEQUENCEDIALOGPRESETTAB_H

#include <QLabel>
#include <QTreeWidget>
#include <QWidget>

#include "presetmanager.h"
#include "sequencepreset.h"

namespace olive {

class SequenceDialogPresetTab : public QWidget, public PresetManager<SequencePreset>
{
  Q_OBJECT
public:
  SequenceDialogPresetTab(QWidget* parent = nullptr);

  virtual ~SequenceDialogPresetTab() override;

public slots:
  void SaveParametersAsPreset(SequencePreset preset);

signals:
  void PresetChanged(const SequencePreset& preset);

  void PresetAccepted();

private:
  QTreeWidgetItem *CreateFolder(const QString& name);

  QTreeWidgetItem *CreateHDPresetFolder(const QString& name, int width, int height, int divider);

  QTreeWidgetItem *CreateSDPresetFolder(const QString& name, int width, int height, const rational &frame_rate, const rational& standard_par, const rational& wide_par, int divider);

  QTreeWidgetItem* GetSelectedItem();
  QTreeWidgetItem* GetSelectedCustomPreset();

  void AddStandardItem(QTreeWidgetItem* folder, Preset* preset, const QString &description = QString());

  void AddCustomItem(QTreeWidgetItem* folder, Preset* preset, int index, const QString& description = QString());

  void AddItemInternal(QTreeWidgetItem* folder, Preset* preset, bool is_custom, int index, const QString& description = QString());

  QTreeWidget* preset_tree_;

  QTreeWidgetItem* my_presets_folder_;

  QVector<Preset*> default_preset_data_;

private slots:
  void SelectedItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

  void ItemDoubleClicked(QTreeWidgetItem *item, int column);

  void ShowContextMenu();

  void DeleteSelectedPreset();

};

}

#endif // SEQUENCEDIALOGPRESETTAB_H
