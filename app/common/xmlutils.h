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

#ifndef XMLREADLOOP_H
#define XMLREADLOOP_H

#include <QUndoCommand>
#include <QXmlStreamReader>

#include "project/item/footage/stream.h"

OLIVE_NAMESPACE_ENTER

class Block;
class Node;
class NodeParam;
class NodeInput;
class NodeOutput;
class Item;

#define XMLAttributeLoop(reader, item) \
  QXmlStreamAttributes __attributes = reader->attributes(); \
  foreach (const QXmlStreamAttribute& item, __attributes)

struct XMLNodeData {
  struct SerializedConnection {
    NodeInput* input;
    quintptr output;
  };

  struct FootageConnection {
    NodeInput* input;
    quintptr footage;
  };

  struct BlockLink {
    Block* block;
    quintptr link;
  };

  QHash<quintptr, Node*> node_ptrs;
  QHash<quintptr, NodeOutput*> output_ptrs;
  QList<SerializedConnection> desired_connections;
  QHash<quintptr, StreamPtr> footage_ptrs;
  QList<FootageConnection> footage_connections;
  QList<BlockLink> block_links;
  QHash<quintptr, Item*> item_ptrs;

};

void XMLConnectNodes(const XMLNodeData& xml_node_data, QUndoCommand* command = nullptr);

bool XMLReadNextStartElement(QXmlStreamReader* reader);

void XMLLinkBlocks(const XMLNodeData& xml_node_data);

OLIVE_NAMESPACE_EXIT

#endif // XMLREADLOOP_H
