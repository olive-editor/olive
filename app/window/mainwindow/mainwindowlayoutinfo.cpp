/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindowlayoutinfo.h"

namespace olive
{

void MainWindowLayoutInfo::toXml(QXmlStreamWriter *writer) const
{
	writer->writeAttribute(QStringLiteral("version"),
						   QString::number(kVersion));

	writer->writeStartElement(QStringLiteral("folders"));

	foreach (Folder *folder, open_folders_) {
		writer->writeTextElement(
			QStringLiteral("folder"),
			QString::number(reinterpret_cast<quintptr>(folder)));
	}

	writer->writeEndElement(); // folders

	writer->writeStartElement(QStringLiteral("timeline"));

	foreach (Sequence *sequence, open_sequences_) {
		writer->writeTextElement(
			QStringLiteral("sequence"),
			QString::number(reinterpret_cast<quintptr>(sequence)));
	}

	writer->writeEndElement(); // timeline

	writer->writeStartElement(QStringLiteral("viewers"));

	foreach (ViewerOutput *viewer, open_viewers_) {
		writer->writeTextElement(
			QStringLiteral("viewer"),
			QString::number(reinterpret_cast<quintptr>(viewer)));
	}

	writer->writeEndElement(); // viewers

	writer->writeStartElement(QStringLiteral("data"));

	for (auto it = panel_data_.cbegin(); it != panel_data_.cend(); it++) {
		writer->writeStartElement(QStringLiteral("panel"));

		writer->writeAttribute(QStringLiteral("id"), it->first);

		const PanelWidget::Info &info = it->second;
		for (auto jt = info.cbegin(); jt != info.cend(); jt++) {
			writer->writeStartElement(QStringLiteral("option"));

			writer->writeAttribute(QStringLiteral("name"), jt->first);

			writer->writeCharacters(jt->second);

			writer->writeEndElement(); // option
		}

		writer->writeEndElement(); // panel
	}

	writer->writeEndElement(); // data

	writer->writeTextElement(QStringLiteral("state"),
							 QString(state_.toBase64()));
}

MainWindowLayoutInfo
MainWindowLayoutInfo::fromXml(QXmlStreamReader *reader,
							  const QHash<quintptr, Node *> &node_ptrs)
{
	MainWindowLayoutInfo info;

	unsigned int file_version = 0;

	XMLAttributeLoop(reader, attr)
	{
		if (attr.name() == QStringLiteral("version")) {
			file_version = attr.value().toUInt();
		}
	}

	// Really basic version checking, in the future we may use this to parse multiple versions
	if (file_version != kVersion) {
	}

	while (XMLReadNextStartElement(reader)) {
		if (reader->name() == QStringLiteral("folders")) {
			while (XMLReadNextStartElement(reader)) {
				if (reader->name() == QStringLiteral("folder")) {
					quintptr item_id = reader->readElementText().toULongLong();

					Folder *open_item =
						static_cast<Folder *>(node_ptrs.value(item_id));
					info.open_folders_.push_back(open_item);
				} else {
					reader->skipCurrentElement();
				}
			}

		} else if (reader->name() == QStringLiteral("timeline")) {
			while (XMLReadNextStartElement(reader)) {
				if (reader->name() == QStringLiteral("sequence")) {
					quintptr item_id = reader->readElementText().toULongLong();

					Sequence *open_seq =
						static_cast<Sequence *>(node_ptrs.value(item_id));
					info.open_sequences_.push_back(open_seq);
				} else {
					reader->skipCurrentElement();
				}
			}

		} else if (reader->name() == QStringLiteral("viewers")) {
			while (XMLReadNextStartElement(reader)) {
				if (reader->name() == QStringLiteral("viewer")) {
					quintptr item_id = reader->readElementText().toULongLong();

					ViewerOutput *open_viewer =
						static_cast<ViewerOutput *>(node_ptrs.value(item_id));
					info.open_viewers_.push_back(open_viewer);
				} else {
					reader->skipCurrentElement();
				}
			}

		} else if (reader->name() == QStringLiteral("state")) {
			info.state_ =
				QByteArray::fromBase64(reader->readElementText().toLatin1());

		} else if (reader->name() == QStringLiteral("data")) {
			while (XMLReadNextStartElement(reader)) {
				if (reader->name() == QStringLiteral("panel")) {
					QString id;
					XMLAttributeLoop(reader, attr)
					{
						if (attr.name() == QStringLiteral("id")) {
							id = attr.value().toString();
						}
					}

					if (!id.isEmpty()) {
						PanelWidget::Info i;

						while (XMLReadNextStartElement(reader)) {
							if (reader->name() == QStringLiteral("option")) {
								QString name;

								XMLAttributeLoop(reader, attr)
								{
									if (attr.name() == QStringLiteral("name")) {
										name = attr.value().toString();
									}
								}

								if (!name.isEmpty()) {
									i[name] = reader->readElementText();
								}
							} else {
								reader->skipCurrentElement();
							}
						}

						info.panel_data_[id] = i;
					}

				} else {
					reader->skipCurrentElement();
				}
			}

		} else {
			reader->skipCurrentElement();
		}
	}

	return info;
}

void MainWindowLayoutInfo::add_folder(olive::Folder *f)
{
	open_folders_.push_back(f);
}

void MainWindowLayoutInfo::add_sequence(Sequence *seq)
{
	open_sequences_.push_back(seq);
}

void MainWindowLayoutInfo::add_viewer(ViewerOutput *viewer)
{
	open_viewers_.push_back(viewer);
}

void MainWindowLayoutInfo::set_panel_data(const QString &id,
										  const PanelWidget::Info &data)
{
	panel_data_[id] = data;
}

void MainWindowLayoutInfo::move_panel_data(const QString &old,
										   const QString &now)
{
	PanelWidget::Info tmp = panel_data_.at(old);
	panel_data_.erase(old);
	panel_data_[now] = tmp;
}

void MainWindowLayoutInfo::set_state(const QByteArray &layout)
{
	state_ = layout;
}

}
