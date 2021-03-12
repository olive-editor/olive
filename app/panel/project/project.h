/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

namespace olive {

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

  QModelIndex get_root_index() const;

  void set_root(Folder* item);

  QVector<Node *> SelectedItems() const;

  Folder* GetSelectedFolder() const;

  virtual QVector<Footage *> GetSelectedFootage() const override;

  ProjectViewModel* model() const;

  virtual void SelectAll() override;
  virtual void DeselectAll() override;

  virtual void Insert() override;
  virtual void Overwrite() override;

  virtual void DeleteSelected() override;

public slots:
  void Edit(Node *item);

signals:
  void ProjectNameChanged();

private:
  virtual void Retranslate() override;

  ProjectExplorer* explorer_;

private slots:
  void ItemDoubleClickSlot(Node *item);

  void ShowNewMenu();

  void UpdateSubtitle();

  void SaveConnectedProject();

};

}

#endif // PROJECT_PANEL_H
