#ifndef XMLREADLOOP_H
#define XMLREADLOOP_H

#include <QXmlStreamReader>

#include "node/node.h"

#define XMLAttributeLoop(reader, item) \
  QXmlStreamAttributes __attributes = reader->attributes(); \
  foreach (const QXmlStreamAttribute& item, __attributes)

Node *XMLLoadNode(QXmlStreamReader* reader);

void XMLConnectNodes(const QHash<quintptr, NodeOutput *> &output_ptrs, const QList<NodeParam::SerializedConnection> &desired_connections);

bool XMLReadNextStartElement(QXmlStreamReader* reader);

#endif // XMLREADLOOP_H
