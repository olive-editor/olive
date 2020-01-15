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

#include "common/xmlreadloop.h"
#include "project/item/footage/footage.h"
#include "project/item/sequence/sequence.h"
#include "ui/icons/icons.h"

Folder::Folder()
{
}

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

void Folder::Load(QXmlStreamReader *reader, QHash<quintptr, StreamPtr> &footage_ptrs, QList<NodeParam::FootageConnection>& footage_connections)
{
  XMLAttributeLoop(reader, attr) {
    if (attr.name() == "name") {
      set_name(attr.value().toString());
    }
  }

  XMLReadLoop(reader, "folder") {
    if (reader->isStartElement()) {
      ItemPtr child;

      if (reader->name() == "folder") {
        child = std::make_shared<Folder>();
      } else if (reader->name() == "footage") {
        child = std::make_shared<Footage>();
      } else if (reader->name() == "sequence") {
        child = std::make_shared<Sequence>();
      } else {
        continue;
      }

      add_child(child);
      child->Load(reader, footage_ptrs, footage_connections);
    }
  }
}

void Folder::Save(QXmlStreamWriter *writer) const
{
  writer->writeStartElement("folder");

  writer->writeAttribute("name", name());

  foreach (ItemPtr child, children()) {
    child->Save(writer);
  }

  writer->writeEndElement(); // folder
}
