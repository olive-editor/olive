/***
  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team
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

#include "keyframecopypaste.h"

#include "core.h"
#include "widget/timebased/timebasedwidget.h"

namespace olive {
void KeyframeCopyPasteService::CopyKeyframesToClipboard(QVector<NodeKeyframe*> selected_keyframes, void* userdata)
{
  QString copy_str;

  QXmlStreamWriter writer(&copy_str);
  writer.setAutoFormatting(true);

  writer.writeStartDocument();
  writer.writeStartElement(QStringLiteral("olive"));

  writer.writeTextElement(QStringLiteral("version"), QString::number(Core::kProjectVersion));

  writer.writeStartElement(QStringLiteral("keyframes"));

  foreach(NodeKeyframe* keyframe, selected_keyframes) {
    writer.writeStartElement(QStringLiteral("node"));
    writer.writeAttribute(QStringLiteral("node_type"), keyframe->parent()->id());
    NodeValue::Type data_type = keyframe->parent()->GetInputDataType(keyframe->input());
    int track =  keyframe->track();
    writer.writeStartElement(QStringLiteral("track"));
    writer.writeAttribute(QStringLiteral("track"), QString::number(track));

	keyframe->parent()->SaveKeyframeData(&writer, keyframe, data_type);

    writer.writeEndElement(); // track
    writer.writeEndElement();  // node
  }

  writer.writeEndElement();  // keyframes
  writer.writeEndElement();  // olive
  writer.writeEndDocument();

  Core::CopyStringToClipboard(copy_str);
}

void KeyframeCopyPasteService::PasteKeyframesFromClipboard(MultiUndoCommand* command) {
  // use NodeParamInsertKeyframeCommand::NodeParamInsertKeyframeCommand
  QString clipboard = Core::PasteStringFromClipboard();

  if (clipboard.isEmpty()) {
    return;
  }

  QXmlStreamReader reader(clipboard);
  uint data_version = 0;

  QVector<TimelineMarker*> pasted_markers;

  while (XMLReadNextStartElement(&reader)) {
    if (reader.name() == QStringLiteral("olive")) {
      while (XMLReadNextStartElement(&reader)) {
        if (reader.name() == QStringLiteral("version")) {
          data_version = reader.readElementText().toUInt();
        } else if (reader.name() == QStringLiteral("keyframes")) {
          while (XMLReadNextStartElement(&reader)) {
            if (data_version == 0) {
              return;
            }
            XMLAttributeLoop((&reader), attr) {
              if (attr.name() == QStringLiteral("name")) {
                // name = attr.value().toString();
              }
            }
          }
        }
      }
    }
  }
}

} // namespace olive
