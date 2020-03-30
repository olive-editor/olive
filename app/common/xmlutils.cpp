#include "xmlutils.h"

#include "node/factory.h"
#include "widget/nodeview/nodeviewundo.h"

Node* XMLLoadNode(QXmlStreamReader* reader) {
  QString node_id;
  quintptr node_ptr = 0;

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("id")) {
      node_id = attr.value().toString();
    } else if (attr.name() == QStringLiteral("ptr")) {
      node_ptr = attr.value().toULongLong();
    }
  }

  if (node_id.isEmpty()) {
    qWarning() << "Found node with no ID";
    return nullptr;
  }

  Node* node = NodeFactory::CreateFromID(node_id);

  if (node) {
    node->setProperty("xml_ptr", node_ptr);
  } else {
    qWarning() << "Failed to load" << node_id << "- no node with that ID is installed";
  }

  return node;
}

void XMLConnectNodes(const QHash<quintptr, NodeOutput*>& output_ptrs, const QList<XMLNodeData::SerializedConnection>& desired_connections, QUndoCommand *command)
{
  foreach (const XMLNodeData::SerializedConnection& con, desired_connections) {
    NodeOutput* out = output_ptrs.value(con.output);

    if (out) {
      if (command) {
        new NodeEdgeAddCommand(out, con.input, command);
      } else {
        NodeParam::ConnectEdge(out, con.input);
      }
    }
  }
}

bool XMLReadNextStartElement(QXmlStreamReader *reader)
{
  QXmlStreamReader::TokenType token;

  while ((token = reader->readNext()) != QXmlStreamReader::Invalid
         && token != QXmlStreamReader::EndDocument) {
    if (reader->isEndElement()) {
      return false;
    } else if (reader->isStartElement()) {
      return true;
    }
  }

  return false;
}
