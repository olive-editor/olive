#include "xmlutils.h"

#include "node/factory.h"

Node* XMLLoadNode(QXmlStreamReader* reader) {
  QString node_id;

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == "id") {
      node_id = attr.value().toString();

      // Currently the only thing we need
      break;
    }
  }

  if (node_id.isEmpty()) {
    qWarning() << "Found node with no ID";
    return nullptr;
  }

  Node* node = NodeFactory::CreateFromID(node_id);

  if (!node) {
    qWarning() << "Failed to load" << node_id << "- no node with that ID is installed";
  }

  return node;
}

void XMLConnectNodes(const QHash<quintptr, NodeOutput*>& output_ptrs, const QList<NodeParam::SerializedConnection>& desired_connections)
{
  foreach (const NodeParam::SerializedConnection& con, desired_connections) {
    NodeParam::ConnectEdge(output_ptrs.value(con.output),
                           con.input);
  }
}
