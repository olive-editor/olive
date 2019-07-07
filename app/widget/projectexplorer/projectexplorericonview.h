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

#ifndef PROJECTEXPLORERICONVIEW_H
#define PROJECTEXPLORERICONVIEW_H

#include "projectexplorerlistviewbase.h"
#include "projectexplorericonviewitemdelegate.h"

/**
 * @brief The view widget used when ProjectExplorer is in Icon View
 */
class ProjectExplorerIconView : public ProjectExplorerListViewBase
{
  Q_OBJECT
public:
  ProjectExplorerIconView(QWidget* parent);

private:
  ProjectExplorerIconViewItemDelegate delegate_;
};

#endif // PROJECTEXPLORERICONVIEW_H
