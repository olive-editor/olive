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
#include "node/project/project.h"
#include "node/project/sequence/sequence.h"
#include "node/project/serializer/serializer.h"

namespace olive {

class NodeCopyPasteService
{
public:
  NodeCopyPasteService() = default;

protected:
  void CopyNodesToClipboard(QVector<Node *> nodes, void* userdata = nullptr);

  void PasteNodesFromClipboard(void* userdata = nullptr);

  virtual void CopyNodesToClipboardCallback(const QVector<Node*> &nodes, ProjectSerializer::SaveData *data, void *userdata){}

  virtual void PasteNodesToClipboardCallback(const QVector<Node*> &nodes, const ProjectSerializer::LoadData &load_data, void *userdata){}

};

}

#endif // NODECOPYPASTEWIDGET_H
