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
    QString output_param;
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
    InputFlag custom_flags;
    type_t data_type;
    value_t default_val;
    QHash<QString, value_t> custom_properties;
  };

  QMap<Node *, QMap<quintptr, Node::Position> > positions;
  QHash<quintptr, Node*> node_ptrs;
  QList<SerializedConnection> desired_connections;
  QList<BlockLink> block_links;
  QVector<GroupLink> group_input_links;
  QHash<NodeGroup*, quintptr> group_output_links;

};

}

#endif // SERIALIZEDDATA_H
