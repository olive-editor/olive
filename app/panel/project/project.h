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

#ifndef PROJECT_PANEL_H
#define PROJECT_PANEL_H

#include "footagemanagementpanel.h"
#include "project/project.h"
#include "widget/panel/panel.h"
#include "widget/projectexplorer/projectexplorer.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A PanelWidget wrapper around a ProjectExplorer and a ProjectToolbar
 */
class ProjectPanel : public PanelWidget, public FootageManagementPanel
{
  Q_OBJECT
public:
  ProjectPanel(QWidget* parent);

  Project* project() const;
  void set_project(Project* p);

  void set_root(Item* item);

  QList<Item*> SelectedItems() const;

  Folder* GetSelectedFolder() const;

  virtual QList<Footage*> GetSelectedFootage() const override;

  ProjectViewModel* model() const;

  virtual void SelectAll() override;
  virtual void DeselectAll() override;

  virtual void Insert() override;
  virtual void Overwrite() override;

  virtual void DeleteSelected() override;

public slots:
  void Edit(Item *item);

signals:
  void ProjectNameChanged();

private:
  virtual void Retranslate() override;

  ProjectExplorer* explorer_;

private slots:
  void ItemDoubleClickSlot(Item* item);

  void ShowNewMenu();

  void UpdateSubtitle();

};

OLIVE_NAMESPACE_EXIT

#endif // PROJECT_PANEL_H
