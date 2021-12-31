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

#include "nodecopypaste.h"

#include <QMessageBox>

#include "core.h"
#include "node/factory.h"
#include "widget/nodeview/nodeviewundo.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

void NodeCopyPasteService::CopyNodesToClipboard(QVector<Node *> nodes, void *userdata)
{
  QString copy_str;

  QXmlStreamWriter writer(&copy_str);
  writer.setAutoFormatting(true);

  writer.writeStartDocument();
  writer.writeStartElement(QStringLiteral("olive"));

  writer.writeTextElement(QStringLiteral("version"), QString::number(Core::kProjectVersion));

  writer.writeStartElement(QStringLiteral("nodes"));
  for (int i=0; i<nodes.size(); i++) {
    Node *n = nodes.at(i);

    writer.writeStartElement(QStringLiteral("node"));
    writer.writeAttribute(QStringLiteral("id"), n->id());
    n->Save(&writer);
    writer.writeEndElement(); // node

    // If this is a group, add the child nodes too
    if (NodeGroup *g = dynamic_cast<NodeGroup*>(n)) {
      for (auto it=g->GetContextPositions().cbegin(); it!=g->GetContextPositions().cend(); it++) {
        if (!nodes.contains(it.key())) {
          nodes.append(it.key());
        }
      }
    }
  }
  writer.writeEndElement(); // nodes

  writer.writeStartElement(QStringLiteral("contexts"));
  foreach (Node* n, nodes) {
    // Determine if this node is a context
    if (!n->GetContextPositions().isEmpty()) {
      writer.writeStartElement(QStringLiteral("context"));
      writer.writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(n)));

      const Node::PositionMap &map = n->GetContextPositions();
      for (auto it=map.cbegin(); it!=map.cend(); it++) {
        writer.writeStartElement(QStringLiteral("node"));
        Project::SavePosition(&writer, it.key(), it.value());
        writer.writeEndElement(); // node
      }

      writer.writeEndElement(); // context
    }
  }
  writer.writeEndElement(); // contexts

  writer.writeStartElement(QStringLiteral("custom"));
  CopyNodesToClipboardInternal(&writer, nodes, userdata);
  writer.writeEndElement(); // custom

  writer.writeEndElement(); // olive
  writer.writeEndDocument();

  Core::CopyStringToClipboard(copy_str);
}

QVector<Node *> NodeCopyPasteService::PasteNodesFromClipboard(NodeGraph *graph, MultiUndoCommand* command, void *userdata)
{
  QString clipboard = Core::PasteStringFromClipboard();

  if (clipboard.isEmpty()) {
    return QVector<Node*>();
  }

  QXmlStreamReader reader(clipboard);
  uint data_version = 0;

  QVector<Node*> pasted_nodes;
  XMLNodeData xml_node_data;
  QMap<quintptr, QMap<quintptr, Node::Position> > pasted_contexts;

  while (XMLReadNextStartElement(&reader)) {
    if (reader.name() == QStringLiteral("olive")) {
      // Default to current version - this may not be desirable?

      while (XMLReadNextStartElement(&reader)) {
        if (reader.name() == QStringLiteral("version")) {
          data_version = reader.readElementText().toUInt();
        } else if (reader.name() == QStringLiteral("nodes")) {
          while (XMLReadNextStartElement(&reader)) {
            if (reader.name() == QStringLiteral("node")) {
              if (data_version == 0) {
                // Refuse to create any nodes if we didn't get a version string
                return QVector<Node*>();
              }

              Node* node = nullptr;

              XMLAttributeLoop((&reader), attr) {
                if (attr.name() == QStringLiteral("id")) {
                  node = NodeFactory::CreateFromID(attr.value().toString());
                  break;
                }
              }

              if (node) {
                node->Load(&reader, xml_node_data, data_version, nullptr);

                pasted_nodes.append(node);
              }
            } else {
              reader.skipCurrentElement();
            }
          }
        } else if (reader.name() == QStringLiteral("contexts")) {
          while (XMLReadNextStartElement(&reader)) {
            if (reader.name() == QStringLiteral("context")) {
              // Get context ptr
              QMap<quintptr, Node::Position> map;
              quintptr context_ptr = 0;
              XMLAttributeLoop((&reader), attr) {
                if (attr.name() == QStringLiteral("ptr")) {
                  context_ptr = attr.value().toULongLong();
                  break;
                }
              }

              // Find nodes section
              while (XMLReadNextStartElement(&reader)) {
                if (reader.name() == QStringLiteral("node")) {
                  quintptr node_ptr;
                  Node::Position node_pos;

                  if (Project::LoadPosition(&reader, &node_ptr, &node_pos)) {
                    map.insert(node_ptr, node_pos);
                  }
                } else {
                  reader.skipCurrentElement();
                }
              }

              // Insert
              if (!map.isEmpty()) {
                pasted_contexts.insert(context_ptr, map);
              }
            } else {
              reader.skipCurrentElement();
            }
          }
        } else if (reader.name() == QStringLiteral("custom")) {
          if (data_version == 0) {
            // Refuse to create any nodes if we didn't get a version string
            return QVector<Node*>();
          }

          PasteNodesFromClipboardInternal(&reader, xml_node_data, userdata);
        } else {
          reader.skipCurrentElement();
        }
      }
    } else {
      reader.skipCurrentElement();
    }
  }

  if (pasted_nodes.isEmpty()) {
    // If we passed through the whole string and there were no nodes, it must not be data for us after all
    return QVector<Node*>();
  }

  // If we have some nodes AND the XML data was malformed, the user should probably know
  if (reader.hasError()) {
    // Delete all nodes so this is a no-op
    foreach (Node* n, pasted_nodes) {
      delete n;
    }

    // If this was NOT an internal error, we assume it's an XML error that the user needs to know about
    QMessageBox::critical(Core::instance()->main_window(),
                          QCoreApplication::translate("NodeCopyPasteWidget", "Error pasting nodes"),
                          QCoreApplication::translate("NodeCopyPasteWidget", "Failed to paste nodes: %1").arg(reader.errorString()),
                          QMessageBox::Ok);

    return QVector<Node*>();
  }

  // Add all nodes to graph
  foreach (Node* n, pasted_nodes) {
    if (command) {
      command->add_child(new NodeAddCommand(graph, n));
    } else {
      n->setParent(graph);
    }
  }

  // Process contexts
  for (auto it=pasted_contexts.cbegin(); it!=pasted_contexts.cend(); it++) {
    Node *context = xml_node_data.node_ptrs.value(it.key());
    if (context) {
      auto map = it.value();
      for (auto jt=map.cbegin(); jt!=map.cend(); jt++) {
        Node *subnode = xml_node_data.node_ptrs.value(jt.key());
        if (subnode) {
          if (command) {
            command->add_child(new NodeSetPositionCommand(subnode, context, jt.value()));
          } else {
            context->SetNodePositionInContext(subnode, jt.value());
          }
        }
      }
    }
  }

  // Make connections
  xml_node_data.PostConnect(data_version, command);

  return pasted_nodes;
}

void NodeCopyPasteService::CopyNodesToClipboardInternal(QXmlStreamWriter*, const QVector<Node *> &, void*)
{
}

void NodeCopyPasteService::PasteNodesFromClipboardInternal(QXmlStreamReader* reader, XMLNodeData &xml_node_data, void*)
{
  Q_UNUSED(xml_node_data)
  reader->skipCurrentElement();
}

}
