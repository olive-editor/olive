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
	qint64 start_index = stream.characterOffset();
	qint64 end_index = start_index;
	while (!stream.atEnd() && !(stream.name() == tag && stream.isEndElement())) {
		end_index = stream.characterOffset();
		stream.readNext();
	}
	qint64 passage_length = end_index - start_index;
	if (passage_length > 0) {
		// store xml data verbatim
		QFile* device = static_cast<QFile*>(stream.device());

		QFile passage_get(device->fileName());
		if (passage_get.open(QFile::ReadOnly)) {
			passage_get.seek(start_index);
			bytes = passage_get.read(passage_length);
			int passage_end = bytes.lastIndexOf('>')+1;
			bytes.remove(passage_end, bytes.size()-passage_end);
			passage_get.close();
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
