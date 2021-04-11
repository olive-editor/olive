#include "mainwindowlayoutinfo.h"

namespace olive {

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

  foreach (const OpenSequence& sequence, open_sequences_) {
    writer->writeTextElement(QStringLiteral("sequence"),
                             QString::number(reinterpret_cast<quintptr>(sequence.sequence)));

    writer->writeTextElement(QStringLiteral("state"),
                             QString(sequence.panel_state.toBase64()));
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

          Folder* open_item = static_cast<Folder*>(xml_data.node_ptrs.value(item_id));
          info.open_folders_.append(open_item);
        } else {
          reader->skipCurrentElement();
        }
      }

    } else if (reader->name() == QStringLiteral("timeline")) {

      Sequence* open_seq = nullptr;
      QByteArray tl_state;

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("sequence")) {
          quintptr item_id = reader->readElementText().toULongLong();

          open_seq = static_cast<Sequence*>(xml_data.node_ptrs.value(item_id));
        } else if (reader->name() == QStringLiteral("state")) {
          tl_state = QByteArray::fromBase64(reader->readElementText().toUtf8());
        } else {
          reader->skipCurrentElement();
        }
      }

      if (open_seq) {
        info.open_sequences_.append({open_seq, tl_state});
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

void MainWindowLayoutInfo::add_sequence(const OpenSequence &seq)
{
  open_sequences_.append(seq);
}

void MainWindowLayoutInfo::set_state(const QByteArray &layout)
{
  state_ = layout;
}

}
