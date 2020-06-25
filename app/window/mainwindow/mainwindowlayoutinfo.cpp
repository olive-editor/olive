#include "mainwindowlayoutinfo.h"

OLIVE_NAMESPACE_ENTER

void MainWindowLayoutInfo::toXml(QXmlStreamWriter *writer) const
{
  writer->writeStartElement(QStringLiteral("layout"));

  writer->writeStartElement(QStringLiteral("folders"));

  foreach (Folder* folder, open_folders_) {
    writer->writeTextElement(QStringLiteral("folder"),
                             QString::number(reinterpret_cast<quintptr>(folder)));
  }

  writer->writeEndElement(); // folders

  writer->writeStartElement(QStringLiteral("timeline"));

  foreach (Sequence* sequence, open_sequences_) {
    writer->writeTextElement(QStringLiteral("sequence"),
                             QString::number(reinterpret_cast<quintptr>(sequence)));
  }

  writer->writeEndElement(); // timeline

  writer->writeTextElement(QStringLiteral("state"), QString(state_.toBase64()));

  writer->writeEndElement(); // layout
}

MainWindowLayoutInfo MainWindowLayoutInfo::fromXml(QXmlStreamReader *reader, XMLNodeData &xml_data)
{
  MainWindowLayoutInfo info;

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("folders")) {

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("folder")) {
          quintptr item_id = reader->readElementText().toULongLong();

          Item* open_item = xml_data.item_ptrs.value(item_id);

          if (open_item) {
            info.open_folders_.append(static_cast<Folder*>(open_item));
          }
        } else {
          reader->skipCurrentElement();
        }
      }

    } else if (reader->name() == QStringLiteral("timeline")) {

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("sequence")) {
          quintptr item_id = reader->readElementText().toULongLong();

          Sequence* open_seq = dynamic_cast<Sequence*>(xml_data.item_ptrs.value(item_id));

          if (open_seq) {
            info.open_sequences_.append(open_seq);
          }
        } else {
          reader->skipCurrentElement();
        }
      }

    } else if (reader->name() == QStringLiteral("state")) {

      info.state_ = QByteArray::fromBase64(reader->readElementText().toLatin1());

    } else {
      reader->skipCurrentElement();
    }
  }

  return info;
}

void MainWindowLayoutInfo::add_folder(olive::Folder *f)
{
  open_folders_.append(f);
}

void MainWindowLayoutInfo::add_sequence(Sequence *s)
{
  open_sequences_.append(s);
}

void MainWindowLayoutInfo::set_state(const QByteArray &layout)
{
  state_ = layout;
}

OLIVE_NAMESPACE_EXIT
