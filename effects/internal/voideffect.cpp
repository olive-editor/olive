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
#include "debug.h"

VoidEffect::VoidEffect(Clip *c, const QString& n) : Effect(c, nullptr) {
	name = n;
	QString display_name;
	if (n.isEmpty()) {
		display_name = tr("(unknown)");
	} else {
		display_name = n;
	}
	EffectRow* row = add_row(tr("Missing Effect"), false, false);
	row->add_widget(new QLabel(display_name));
	container->setText(display_name);

	void_meta.type = EFFECT_TYPE_EFFECT;
	meta = &void_meta;
}

Effect *VoidEffect::copy(Clip *c) {
	Effect* copy = new VoidEffect(c, name);
	copy->set_enabled(is_enabled());
	copy_field_keyframes(copy);
	return copy;
}

void VoidEffect::load(QXmlStreamReader &stream) {
	QString tag = stream.name().toString();

    QXmlStreamWriter writer(&bytes);

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
	if (!name.isEmpty()) {
		stream.writeAttribute("name", name);
		stream.writeAttribute("enabled", QString::number(is_enabled()));

		// force xml writer to expand <effect> tag, ignored when loading
		stream.writeStartElement("void");
		stream.writeEndElement();

		if (!bytes.isEmpty()) {
			// write stored data
			QIODevice* device = stream.device();
			device->write(bytes);
		}
	}
}
