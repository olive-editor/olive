#ifndef NODETRAVERSER_H
#define NODETRAVERSER_H

#include "codec/decoder.h"
#include "common/cancelableobject.h"
#include "dependency.h"
#include "node/output/track/track.h"
#include "project/item/footage/stream.h"
#include "value.h"

class NodeTraverser : public CancelableObject
{
public:
  NodeTraverser();

  NodeValueTable ProcessNode(const NodeDependency &dep);

protected:
  NodeValueDatabase GenerateDatabase(const Node *node, const TimeRange &range);

  virtual NodeValueTable RenderBlock(const TrackOutput *track, const TimeRange& range);

  NodeValueTable ProcessInput(const NodeInput* input, const TimeRange &range);

  virtual void InputProcessingEvent(NodeInput*, const TimeRange&, NodeValueTable*){}

  virtual void ProcessNodeEvent(const Node*, const TimeRange&, const NodeValueDatabase&, NodeValueTable*){}

};

#endif // NODETRAVERSER_H
