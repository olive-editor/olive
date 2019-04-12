/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "voideffect.h"

#include <QXmlStreamReader>
#include <QLabel>
#include <QFile>

#include "ui/collapsiblewidget.h"
#include "global/debug.h"

VoidEffect::VoidEffect(Clip* c, const QString& n, const QString& id) :
  Node(c),
  display_name_(n),
  id_(id)
{
  if (display_name_.isEmpty()) {
    display_name_ = tr("(unknown)");
  }

  new LabelWidget(this, tr("Missing Effect"), display_name_);
}

QString VoidEffect::name()
{
  return display_name_;
}

QString VoidEffect::id()
{
  return id_;
}

QString VoidEffect::description()
{
  return QString();
}

EffectType VoidEffect::type()
{
  return EFFECT_TYPE_EFFECT;
}

olive::TrackType VoidEffect::subtype()
{
  return olive::kTypeVideo;
}

bool VoidEffect::IsCreatable()
{
  return false;
}

NodePtr VoidEffect::Create(Clip *)
{
  return nullptr;
}

NodePtr VoidEffect::copy(Clip* c) {
  NodePtr copy = std::make_shared<VoidEffect>(c, display_name_, id_);
  copy->SetEnabled(IsEnabled());
  copy_field_keyframes(copy);
  return copy;
}

void VoidEffect::load(QXmlStreamReader &stream) {
  QString tag = stream.name().toString();

  QXmlStreamWriter writer(&bytes_);

  // copy XML from reader to writer
  while (!stream.atEnd() && !(stream.name() == tag && stream.isEndElement())) {
    stream.readNext();

    if (stream.isStartElement()) {
      writer.writeStartElement(stream.name().toString());
    }
    if (stream.isEndElement()) {
      writer.writeEndElement();
    }
    if (stream.isCharacters()) {
      writer.writeCharacters(stream.text().toString());
    }
    for (int i=0;i<stream.attributes().size();i++) {
      writer.writeAttribute(stream.attributes().at(i));
    }
  }
}

void VoidEffect::save(QXmlStreamWriter &stream) {
  if (!display_name_.isEmpty()) {
    stream.writeAttribute("name", display_name_);
    stream.writeAttribute("enabled", QString::number(IsEnabled()));

    // force xml writer to expand <effect> tag, ignored when loading
    stream.writeStartElement("void");
    stream.writeEndElement();

    if (!bytes_.isEmpty()) {
      // write stored data
      QIODevice* device = stream.device();
      device->write(bytes_);
    }
  }
}
