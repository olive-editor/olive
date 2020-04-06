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

#ifndef NODETRAVERSER_H
#define NODETRAVERSER_H

#include "codec/decoder.h"
#include "common/cancelableobject.h"
#include "dependency.h"
#include "node/output/track/track.h"
#include "project/item/footage/stream.h"
#include "value.h"

OLIVE_NAMESPACE_ENTER

class NodeTraverser : public CancelableObject
{
public:
  NodeTraverser() = default;

  NodeValueTable ProcessNode(const NodeDependency &dep);

protected:
  NodeValueDatabase GenerateDatabase(const Node *node, const TimeRange &range);

  virtual NodeValueTable RenderBlock(const TrackOutput *track, const TimeRange& range);

  NodeValueTable ProcessInput(const NodeInput* input, const TimeRange &range);

  virtual void InputProcessingEvent(NodeInput*, const TimeRange&, NodeValueTable*){}

  virtual void ProcessNodeEvent(const Node*, const TimeRange&, const NodeValueDatabase&, NodeValueTable*){}

};

OLIVE_NAMESPACE_EXIT

#endif // NODETRAVERSER_H
