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

#ifndef NODETABLEVIEW_H
#define NODETABLEVIEW_H

#include <QTreeWidget>

#include "node/node.h"

namespace olive {

class NodeTableView : public QTreeWidget
{
  Q_OBJECT
public:
  NodeTableView(QWidget* parent = nullptr);

  void SelectNodes(const QVector<Node *> &nodes);

  void DeselectNodes(const QVector<Node*>& nodes);

  void SetTime(const rational& time);

private:
  QMap<Node*, QTreeWidgetItem*> top_level_item_map_;

  rational last_time_;

};

}

#endif // NODETABLEVIEW_H
