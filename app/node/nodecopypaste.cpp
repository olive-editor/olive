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
#include "node/project/serializer/serializer.h"
#include "widget/nodeview/nodeviewundo.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

void NodeCopyPasteService::CopyNodesToClipboard(QVector<Node *> nodes, void *userdata)
{
  QString copy_str;

  QXmlStreamWriter writer(&copy_str);

  // For any groups, add children
  for (int i=0; i<nodes.size(); i++) {
    // If this is a group, add the child nodes too
    if (NodeGroup *g = dynamic_cast<NodeGroup*>(nodes.at(i))) {
      for (auto it=g->GetContextPositions().cbegin(); it!=g->GetContextPositions().cend(); it++) {
        if (!nodes.contains(it.key())) {
          nodes.append(it.key());
        }
      }
    }
  }

  ProjectSerializer::Save(nodes.first()->project(), &writer, nodes);

  /*writer.writeStartElement(QStringLiteral("custom"));
  CopyNodesToClipboardInternal(&writer, nodes, userdata);
  writer.writeEndElement(); // custom*/

  Core::CopyStringToClipboard(copy_str);
}

QVector<Node *> NodeCopyPasteService::PasteNodesFromClipboard(Project *project, MultiUndoCommand* command, void *userdata)
{
  QVector<Node*> pasted_nodes;

  QString clipboard = Core::PasteStringFromClipboard();
  if (clipboard.isEmpty()) {
    return pasted_nodes;
  }

  QXmlStreamReader reader(clipboard);

  Project temp;
  ProjectSerializer::Load(&temp, &reader);

  foreach (Node *n, temp.nodes()) {
    if (!temp.default_nodes().contains(n)) {
      n->setParent(nullptr);
      pasted_nodes.append(n);
    }
  }

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
