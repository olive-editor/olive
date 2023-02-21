/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#ifndef SERIALIZEDDATA_H
#define SERIALIZEDDATA_H

#include <QHash>
#include <QVariant>

#include "node.h"

namespace olive {

class NodeGroup;

struct SerializedData {
  struct SerializedConnection {
    NodeInput input;
    quintptr output_node;
  };

  struct BlockLink {
    Node* block;
    quintptr link;
  };

  struct GroupLink {
    NodeGroup *group;
    QString passthrough_id;
    quintptr input_node;
    QString input_id;
    int input_element;
    QString custom_name;
    InputFlags custom_flags;
    NodeValue::Type data_type;
    QVariant default_val;
    QHash<QString, QVariant> custom_properties;
  };

  QMap<quintptr, QMap<quintptr, Node::Position> > positions;
  QHash<quintptr, Node*> node_ptrs;
  QList<SerializedConnection> desired_connections;
  QList<BlockLink> block_links;
  QVector<GroupLink> group_input_links;
  QHash<NodeGroup*, quintptr> group_output_links;
  QHash<Node*, QUuid> node_uuids;

};

}

#endif // SERIALIZEDDATA_H
