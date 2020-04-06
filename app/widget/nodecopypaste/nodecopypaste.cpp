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

#include "nodecopypaste.h"

#include <QMessageBox>

#include "core.h"
#include "widget/nodeview/nodeviewundo.h"
#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

void NodeCopyPasteWidget::CopyNodesToClipboard(const QList<Node *> &nodes, void *userdata)
{
  QString copy_str;

  QXmlStreamWriter writer(&copy_str);
  writer.setAutoFormatting(true);

  writer.writeStartDocument();
  writer.writeStartElement(QStringLiteral("olive"));

  foreach (Node* n, nodes) {
    n->Save(&writer);
  }

  CopyNodesToClipboardInternal(&writer, userdata);

  writer.writeEndElement(); // olive
  writer.writeEndDocument();

  Core::CopyStringToClipboard(copy_str);
}

QList<Node *> NodeCopyPasteWidget::PasteNodesFromClipboard(Sequence *graph, QUndoCommand* command, void *userdata)
{
  QString clipboard = Core::PasteStringFromClipboard();

  if (clipboard.isEmpty()) {
    return QList<Node*>();
  }

  QXmlStreamReader reader(clipboard);

  QList<Node*> pasted_nodes;
  XMLNodeData xml_node_data;

  while (XMLReadNextStartElement(&reader)) {
    if (reader.name() == QStringLiteral("olive")) {
      while (XMLReadNextStartElement(&reader)) {
        if (reader.name() == QStringLiteral("node")) {
          Node* node = XMLLoadNode(&reader);

          if (node) {
            node->Load(&reader, xml_node_data, nullptr);

            pasted_nodes.append(node);
          }
        } else {
          PasteNodesFromClipboardInternal(&reader, userdata);
        }
      }
    } else {
      reader.skipCurrentElement();
    }
  }

  if (pasted_nodes.isEmpty()) {
    // If we passed through the whole string and there were no nodes, it must not be data for us after all
    return QList<Node*>();
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

    return QList<Node*>();
  }

  // Add all nodes to graph
  foreach (Node* n, pasted_nodes) {
    new NodeAddCommand(graph, n, command);
  }

  // Make connections
  if (!xml_node_data.desired_connections.isEmpty()) {
    XMLConnectNodes(xml_node_data, command);
  }

  // Link blocks
  XMLLinkBlocks(xml_node_data);

  // Connect footage to existing footage if it exists
  if (!xml_node_data.footage_connections.isEmpty()) {
    // Get list of all footage from project
    QList<ItemPtr> footage = graph->project()->get_items_of_type(Item::kFootage);

    if (!footage.isEmpty()) {
      foreach (const XMLNodeData::FootageConnection& con, xml_node_data.footage_connections) {
        if (con.footage) {
          // Assume this is a pointer to a Stream*
          Stream* loaded_stream = reinterpret_cast<Stream*>(con.footage);

          bool found = false;

          foreach (ItemPtr item, footage) {
            const QList<StreamPtr>& streams = std::static_pointer_cast<Footage>(item)->streams();

            foreach (StreamPtr s, streams) {
              if (s.get() == loaded_stream) {
                con.input->set_standard_value(QVariant::fromValue(s));
                found = true;
                break;
              }
            }

            if (found) {
              break;
            }
          }
        }
      }
    }
  }

  return pasted_nodes;
}

void NodeCopyPasteWidget::CopyNodesToClipboardInternal(QXmlStreamWriter*, void*)
{
}

void NodeCopyPasteWidget::PasteNodesFromClipboardInternal(QXmlStreamReader* reader, void*)
{
  reader->skipCurrentElement();
}

OLIVE_NAMESPACE_EXIT
