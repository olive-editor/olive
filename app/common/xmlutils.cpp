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

#include "xmlutils.h"

#include "node/block/block.h"
#include "node/factory.h"
#include "widget/nodeview/nodeviewundo.h"

OLIVE_NAMESPACE_ENTER

Node* XMLLoadNode(QXmlStreamReader* reader)
{
  QString node_id;
  quintptr node_ptr = 0;
  QPointF node_pos;
  QString node_label;

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("id")) {
      node_id = attr.value().toString();
    } else if (attr.name() == QStringLiteral("ptr")) {
      node_ptr = attr.value().toULongLong();
    } else if (attr.name() == QStringLiteral("pos")) {
      QStringList pos = attr.value().toString().split(':');

      // Protection in case this file has been messed with
      if (pos.size() == 2) {
        node_pos.setX(pos.at(0).toDouble());
        node_pos.setY(pos.at(1).toDouble());
      }
    } else if (attr.name() == QStringLiteral("label")) {
      node_label = attr.value().toString();
    }
  }

  if (node_id.isEmpty()) {
    qWarning() << "Found node with no ID";
    return nullptr;
  }

  Node* node = NodeFactory::CreateFromID(node_id);

  if (node) {
    node->setProperty("xml_ptr", node_ptr);
    node->SetPosition(node_pos);
    node->SetLabel(node_label);
  } else {
    qWarning() << "Failed to load" << node_id << "- no node with that ID is installed";
  }

  return node;
}

void XMLConnectNodes(const XMLNodeData &xml_node_data, QUndoCommand *command)
{
  foreach (const XMLNodeData::SerializedConnection& con, xml_node_data.desired_connections) {
    NodeOutput* out = xml_node_data.output_ptrs.value(con.output);

    if (out) {
      if (command) {
        new NodeEdgeAddCommand(out, con.input, command);
      } else {
        NodeParam::ConnectEdge(out, con.input);
      }
    }
  }
}

bool XMLReadNextStartElement(QXmlStreamReader *reader)
{
  QXmlStreamReader::TokenType token;

  while ((token = reader->readNext()) != QXmlStreamReader::Invalid
         && token != QXmlStreamReader::EndDocument) {
    if (reader->isEndElement()) {
      return false;
    } else if (reader->isStartElement()) {
      return true;
    }
  }

  return false;
}

void XMLLinkBlocks(const XMLNodeData &xml_node_data)
{
  foreach (const XMLNodeData::BlockLink& l1, xml_node_data.block_links) {
    foreach (const XMLNodeData::BlockLink& l2, xml_node_data.block_links) {
      if (l1.link == l2.block->property("xml_ptr")) {
        Block::Link(l1.block, l2.block);
        break;
      }
    }
  }
}

OLIVE_NAMESPACE_EXIT
