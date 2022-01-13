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

  ProjectSerializer::SaveData data(nodes.first()->project(), QString(), nodes);

  CopyNodesToClipboardCallback(nodes, &data, userdata);

  ProjectSerializer::Save(&writer, data);

  Core::CopyStringToClipboard(copy_str);
}

void NodeCopyPasteService::PasteNodesFromClipboard(void *userdata)
{
  QString clipboard = Core::PasteStringFromClipboard();
  if (clipboard.isEmpty()) {
    return;
  }

  QXmlStreamReader reader(clipboard);

  Project temp;
  ProjectSerializer::Result res = ProjectSerializer::Load(&temp, &reader);

  if (res.code() != ProjectSerializer::kSuccess) {
    return;
  }

  QVector<Node*> pasted_nodes;
  foreach (Node *n, temp.nodes()) {
    if (!temp.default_nodes().contains(n)) {
      // Move nodes out of Project
      n->setParent(nullptr);
      pasted_nodes.append(n);
    }
  }

  if (pasted_nodes.isEmpty()) {
    return;
  }

  PasteNodesToClipboardCallback(pasted_nodes, res.GetLoadData(), userdata);
}

}
