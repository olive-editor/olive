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

#include "output.h"

#include "node/node.h"

NodeOutput::NodeOutput(const QString &id) :
  NodeParam(id)
{
}

NodeParam::Type NodeOutput::type()
{
  return kOutput;
}

QString NodeOutput::name()
{
  if (name_.isEmpty()) {
    return tr("Output");
  }

  return NodeParam::name();
}

void NodeOutput::Save(QXmlStreamWriter *writer) const
{
  writer->writeStartElement("output");

  writer->writeAttribute("id", id());

  writer->writeAttribute("ptr", QString::number(reinterpret_cast<quintptr>(this)));

  SaveConnections(writer);

  writer->writeEndElement(); // output
}
