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

#include "xmlutils.h"

#include "node/block/block.h"
#include "node/factory.h"
#include "widget/nodeparamview/nodeparamviewundo.h"
#include "widget/nodeview/nodeviewundo.h"
//#include "widget/timelinewidget/undo/timelineundogeneral.h"

namespace olive {

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

void XMLNodeData::PostConnect(uint version, MultiUndoCommand *command) const
{
  foreach (const XMLNodeData::SerializedConnection& con, desired_connections) {
    if (Node *out = node_ptrs.value(con.output_node)) {
      // Use output param as hint tag since we grandfathered those in
      Node::ValueHint hint(con.output_param);

      if (command) {
        command->add_child(new NodeEdgeAddCommand(out, con.input));
      } else {
        Node::ConnectEdge(out, con.input);
      }

      if (version < 210907) {
        /// Deprecated: backwards compatibility only
        if (command) {
          command->add_child(new NodeSetValueHintCommand(con.input, hint));
        } else {
          con.input.node()->SetValueHintForInput(con.input.input(), hint, con.input.element());
        }
      }
    }
  }

  foreach (const XMLNodeData::BlockLink& l, block_links) {
    Node *a = l.block;
    Node *b = node_ptrs.value(l.link);
    if (command) {
      command->add_child(new NodeLinkCommand(a, b, true));
    } else {
      Node::Link(a, b);
    }
  }

  foreach (const XMLNodeData::GroupLink &l, group_input_links) {
    if (Node *input_node = node_ptrs.value(l.input_node)) {
      NodeInput resolved(input_node, l.input_id, l.input_element);
      if (command) {
        command->add_child(new NodeGroupAddInputPassthrough(l.group, resolved));
      } else {
        l.group->AddInputPassthrough(resolved);
      }
    }
  }

  for (auto it=group_output_links.cbegin(); it!=group_output_links.cend(); it++) {
    if (Node *output_node = node_ptrs.value(it.value())) {
      if (command) {
        command->add_child(new NodeGroupSetOutputPassthrough(it.key(), output_node));
      } else {
        it.key()->SetOutputPassthrough(output_node);
      }
    }
  }
}

}
