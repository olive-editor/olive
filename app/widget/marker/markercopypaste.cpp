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

#include "markercopypaste.h"

#include <QMessageBox>

#include "core.h"
#include "markerundo.h"
#include "widget/timebased/timebasedwidget.h"

namespace olive {

void MarkerCopyPasteService::CopyMarkersToClipboard(const QVector<TimelineMarker*>& markers, void* userdata)
{
  QString copy_str;

  QXmlStreamWriter writer(&copy_str);
  writer.setAutoFormatting(true);

  writer.writeStartDocument();
  writer.writeStartElement(QStringLiteral("olive"));

  writer.writeTextElement(QStringLiteral("version"), QString::number(Core::kProjectVersion));

  writer.writeStartElement(QStringLiteral("markers"));

  // Get earliest marker for offset
  rational min_start = RATIONAL_MAX;
  foreach (TimelineMarker* marker, markers) {
    if (marker->time().in() < min_start) {
      min_start = marker->time().in();
    }
  }

  foreach(TimelineMarker * marker, markers) {
    // Cache marker in/out points
    rational in = marker->time().in();
    rational out = marker->time().out();

    // Temporarily set in/out to be relative to the earliest selected marker
    marker->set_time(TimeRange(in - min_start, marker->time().out() - min_start));
	marker->Save(&writer);

    // Reset in/out
    marker->set_time(TimeRange(in, out));
  }


  writer.writeEndElement(); // markers
  writer.writeEndElement();  // olive
  writer.writeEndDocument();

  Core::CopyStringToClipboard(copy_str);
}

void MarkerCopyPasteService::PasteMarkersFromClipboard(TimelineMarkerList* list, MultiUndoCommand* command,
                                                       rational offset) {
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
        } else if (reader.name() == QStringLiteral("markers")) {
          while (XMLReadNextStartElement(&reader)) {
            if (data_version == 0) {
              return;
            }
            QString name;
            int color = -1;
            rational in, out;

            XMLAttributeLoop((&reader), attr) {
              if (attr.name() == QStringLiteral("name")) {
                name = attr.value().toString();
              }

              if (attr.name() == QStringLiteral("color")) {
                color = attr.value().toInt();
              }

              if (attr.name() == QStringLiteral("in")) {
                in = rational::fromString(attr.value().toString());
              }

              if (attr.name() == QStringLiteral("out")) {
                out = rational::fromString(attr.value().toString());
              }
            }

            command->add_child(new MarkerAddCommand(Core::instance()->GetActiveProject(),
                                                                     list,
                                                                     TimeRange(in + offset, out + offset),
                                                                     name,
                                                                     color));
          }
        }
      }
    }
  }

  if (reader.hasError()) {
    // If this was NOT an internal error, we assume it's an XML error that the user needs to know about
    QMessageBox::critical((QWidget*)Core::instance()->main_window(),
                          QCoreApplication::translate("MarkerCopyPasteWidget", "Error pasting markers"),
                          QCoreApplication::translate("MarkerCopyPasteWidget", "Failed to paste markers: %1").arg(reader.errorString()),
                          QMessageBox::Ok);

    return;
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);

}
} //namespace olive
