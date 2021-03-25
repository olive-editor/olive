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

#include "xmlutils.h"

#include "node/block/block.h"
#include "node/factory.h"
#include "widget/nodeview/nodeviewundo.h"

namespace olive {

void XMLConnectNodes(const XMLNodeData &xml_node_data, MultiUndoCommand *command)
{
  foreach (const XMLNodeData::SerializedConnection& con, xml_node_data.desired_connections) {
    NodeOutput out(xml_node_data.node_ptrs.value(con.output_node), con.output);

    if (out.IsValid()) {
      if (command) {
        command->add_child(new NodeEdgeAddCommand(out, con.input));
      } else {
        Node::ConnectEdge(out, con.input);
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
  foreach (const XMLNodeData::BlockLink& l, xml_node_data.block_links) {
    Block::Link(l.block, static_cast<Block*>(xml_node_data.node_ptrs.value(l.link)));
  }
}

}
