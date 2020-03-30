#ifndef XMLREADLOOP_H
#define XMLREADLOOP_H

#include <QUndoCommand>
#include <QXmlStreamReader>

class Node;
class NodeParam;
class NodeInput;
class NodeOutput;

#include "project/item/footage/stream.h"

#define XMLAttributeLoop(reader, item) \
  QXmlStreamAttributes __attributes = reader->attributes(); \
  foreach (const QXmlStreamAttribute& item, __attributes)

Node *XMLLoadNode(QXmlStreamReader* reader);

struct XMLNodeData {
  struct SerializedConnection {
    NodeInput* input;
    quintptr output;
  };

  struct FootageConnection {
    NodeInput* input;
    quintptr footage;
  };

  QHash<quintptr, NodeOutput*> output_ptrs;
  QList<SerializedConnection> desired_connections;
  QHash<quintptr, StreamPtr> footage_ptrs;
  QList<FootageConnection> footage_connections;
};

void XMLConnectNodes(const QHash<quintptr, NodeOutput *> &output_ptrs, const QList<XMLNodeData::SerializedConnection> &desired_connections, QUndoCommand* command = nullptr);

bool XMLReadNextStartElement(QXmlStreamReader* reader);

#endif // XMLREADLOOP_H
