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

#ifndef NODECOPYPASTEWIDGET_H
#define NODECOPYPASTEWIDGET_H

#include <QWidget>
#include <QUndoCommand>

#include "node/node.h"
#include "node/project/sequence/sequence.h"

namespace olive {

class NodeCopyPasteService
{
public:
  NodeCopyPasteService() = default;

protected:
  void CopyNodesToClipboard(const QVector<Node *> &nodes, void* userdata = nullptr);

  QVector<Node*> PasteNodesFromClipboard(NodeGraph *graph, MultiUndoCommand *command, void* userdata = nullptr);

  virtual void CopyNodesToClipboardInternal(QXmlStreamWriter *writer, void* userdata);

  virtual void PasteNodesFromClipboardInternal(QXmlStreamReader *reader, XMLNodeData &xml_node_data, void* userdata);

};

}

#endif // NODECOPYPASTEWIDGET_H
