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

#include "folder.h"

#include "common/xmlutils.h"
#include "project/item/footage/footage.h"
#include "project/item/sequence/sequence.h"
#include "ui/icons/icons.h"

OLIVE_NAMESPACE_ENTER

Item::Type Folder::type() const
{
  return kFolder;
}

bool Folder::CanHaveChildren() const
{
  return true;
}

QIcon Folder::icon()
{
  return icon::Folder;
}

void Folder::Load(QXmlStreamReader *reader, XMLNodeData& xml_node_data, const QAtomicInt *cancelled)
{
  XMLAttributeLoop(reader, attr) {
    if (cancelled && *cancelled) {
      return;
    }

    if (attr.name() == QStringLiteral("name")) {
      set_name(attr.value().toString());
    } else if (attr.name() == QStringLiteral("ptr")) {
      xml_node_data.item_ptrs.insert(attr.value().toULongLong(), this);
    }
  }

  while (XMLReadNextStartElement(reader)) {
    if (cancelled && *cancelled) {
      return;
    }

    ItemPtr child;

    if (reader->name() == QStringLiteral("folder")) {
      child = std::make_shared<Folder>();
    } else if (reader->name() == QStringLiteral("footage")) {
      child = std::make_shared<Footage>();
    } else if (reader->name() == QStringLiteral("sequence")) {
      child = std::make_shared<Sequence>();
    } else {
      reader->skipCurrentElement();
      continue;
    }

    add_child(child);
    child->Load(reader, xml_node_data, cancelled);
  }
}

void Folder::Save(QXmlStreamWriter *writer) const
{
  writer->writeAttribute(QStringLiteral("name"), name());

  writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(this)));

  foreach (ItemPtr child, children()) {
    switch (child->type()) {
    case Item::kFootage:
      writer->writeStartElement(QStringLiteral("footage"));
      break;
    case Item::kSequence:
      writer->writeStartElement(QStringLiteral("sequence"));
      break;
    case Item::kFolder:
      writer->writeStartElement(QStringLiteral("folder"));
      break;
    }

    child->Save(writer);

    writer->writeEndElement(); // footage/folder/sequence
  }
}

OLIVE_NAMESPACE_EXIT
