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

#ifndef NODEGRAPH_H
#define NODEGRAPH_H

#include <QObject>

#include "node/node.h"

class NodeGraph : public QObject
{
  Q_OBJECT
public:
  NodeGraph();

  void AddNode(Node* node);

  const QString& name();
  void set_name(const QString& name);

  QList<Node*> nodes();

signals:
  void EdgeAdded(NodeEdgePtr edge);
  void EdgeRemoved(NodeEdgePtr edge);

private:
  QString name_;
};

#endif // NODEGRAPH_H
