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

#include "project/project.h"
#include "widget/panel/panel.h"
#include "widget/projectexplorer/projectexplorer.h"

/**
 * @brief A PanelWidget wrapper around a ProjectExplorer and a ProjectToolbar
 */
class ProjectPanel : public PanelWidget
{
  Q_OBJECT
public:
  ProjectPanel(QWidget* parent);

  Project* project();
  void set_project(Project* p);

  QList<Item*> SelectedItems();

  Folder* GetSelectedFolder();

  ProjectViewModel* model();

public slots:
  void Edit(Item *item);

protected:
  virtual void changeEvent(QEvent* e) override;

private:
  void Retranslate();

  ProjectExplorer* explorer_;

private slots:
  void ItemDoubleClickSlot(Item* item);

  void ShowNewMenu();
};

#endif // PROJECT_PANEL_H
