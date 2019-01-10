#include "voideffect.h"

#include <QXmlStreamReader>
#include <QLabel>
#include <QFile>

#include "ui/collapsiblewidget.h"
#include "debug.h"

VoidEffect::VoidEffect(Clip *c, const QString& n) : Effect(c, NULL) {
	name = n;
	QString display_name;
	if (n.isEmpty()) {
		display_name = "(unknown)";
	} else {
		display_name = n;
	}
	EffectRow* row = add_row("Missing Effect", false, false);
	row->add_widget(new QLabel(display_name));
	container->setText(display_name);
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
