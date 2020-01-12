#ifndef XMLREADLOOP_H
#define XMLREADLOOP_H

#define XMLReadLoop(reader, section) \
  while (!reader->atEnd() && !(reader->name() == section && reader->isEndElement()) && reader->readNext())

#define XMLAttributeLoop(reader, item) \
  QXmlStreamAttributes __attributes = reader->attributes(); \
  foreach (const QXmlStreamAttribute& item, __attributes)

#endif // XMLREADLOOP_H
