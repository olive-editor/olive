#ifndef XMLREADLOOP_H
#define XMLREADLOOP_H

#include <QUndoCommand>
#include <QXmlStreamReader>

#include "project/item/footage/stream.h"

class Block;
class Node;
class NodeParam;
class NodeInput;
class NodeOutput;

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

  struct BlockLink {
    Block* block;
    quintptr link;
  };

  QHash<quintptr, NodeOutput*> output_ptrs;
  QList<SerializedConnection> desired_connections;
  QHash<quintptr, StreamPtr> footage_ptrs;
  QList<FootageConnection> footage_connections;
  QList<BlockLink> block_links;

};

void XMLConnectNodes(const XMLNodeData& xml_node_data, QUndoCommand* command = nullptr);

bool XMLReadNextStartElement(QXmlStreamReader* reader);

void XMLLinkBlocks(const XMLNodeData& xml_node_data);

#endif // XMLREADLOOP_H
