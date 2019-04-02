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

#include "marker.h"

#include "global/config.h"
#include "undo/undo.h"
#include "undo/undostack.h"
#include "ui/mainwindow.h"
#include "timeline/sequence.h"
#include "timeline/clip.h"
#include "panels/panels.h"
#include "panels/viewer.h"

#include <QInputDialog>
#include <QCoreApplication>

void Marker::Draw(QPainter &p, int x, int y, int bottom, bool selected) {
  const QPoint points[5] = {
    QPoint(x, bottom),
    QPoint(x + MARKER_SIZE, bottom - MARKER_SIZE),
    QPoint(x + MARKER_SIZE, y),
    QPoint(x - MARKER_SIZE, y),
    QPoint(x - MARKER_SIZE, bottom - MARKER_SIZE)
  };
  p.setPen(Qt::black);
  if (selected) {
    p.setBrush(QColor(208, 255, 208));
  } else {
    p.setBrush(QColor(128, 224, 128));
  }
  p.drawPolygon(points, 5);
}

void Marker::SetOnClips(const QVector<Clip *> &clips)
{
  // Don't bother if there are no clips
  if (clips.isEmpty()) {
    return;
  }

  // add_marker is used to determine whether we're adding a marker, depending on whether the user input a marker name
  // however if (config.set_name_with_marker) is true, we don't need a marker name so we just add
  bool add_marker = !olive::config.set_name_with_marker;

  QString marker_name;

  // if Config::set_name_with_marker is true (set above), ask for a marker name
  if (!add_marker) {
    QInputDialog d(olive::MainWindow);
    d.setWindowTitle(QCoreApplication::translate("Marker", "Set Marker"));
    d.setLabelText(QCoreApplication::translate("Marker", "Set clip marker name:"));
    d.setInputMode(QInputDialog::TextInput);
    add_marker = (d.exec() == QDialog::Accepted);
    marker_name = d.textValue();
  }

  // if we've decided to add a marker
  if (add_marker) {

    ComboAction* ca = new ComboAction();

    // add a marker action for each clip
    foreach (Clip* c, clips) {
      ca->append(new AddMarkerAction(&c->get_markers(),
                                     c->track()->sequence()->playhead - c->timeline_in() + c->clip_in(),
                                     marker_name));
    }


    // push action
    olive::undo_stack.push(ca);

    // redraw UI for new markers
    update_ui(false);
    panel_footage_viewer->update_viewer();

  }
}

void Marker::SetOnSequence(Sequence *seq) {

  // Don't bother if there is no sequence
  if (seq == nullptr) {
    return;
  }

  // add_marker is used to determine whether we're adding a marker, depending on whether the user input a marker name
  // however if (config.set_name_with_marker) is true, we don't need a marker name so we just add
  bool add_marker = !olive::config.set_name_with_marker;

  QString marker_name;

  // if Config::set_name_with_marker is true (set above), ask for a marker name
  if (!add_marker) {
    QInputDialog d(olive::MainWindow);
    d.setWindowTitle(QCoreApplication::translate("Marker", "Set Marker"));
    d.setLabelText(QCoreApplication::translate("Marker", "Set sequence marker name:"));
    d.setInputMode(QInputDialog::TextInput);
    add_marker = (d.exec() == QDialog::Accepted);
    marker_name = d.textValue();
  }

  // if we've decided to add a marker
  if (add_marker) {

    ComboAction* ca = new ComboAction();

    // FIXME kind of hacky, we get the correct marker structure from the viewer panel object that the sequence is
    // attached to, as the viewers will give us the footage marker set if its footage rather than the sequence marker
    // set

    if (seq == panel_footage_viewer->seq.get()) {

      // get correct marker reference from footage viewer
      ca->append(new AddMarkerAction(panel_footage_viewer->marker_ref, seq->playhead, marker_name));

    } else if (seq == panel_sequence_viewer->seq.get()) {

      // get correct marker reference from sequence viewer
      ca->append(new AddMarkerAction(panel_sequence_viewer->marker_ref, seq->playhead, marker_name));

    } else {

      // fallback to using markers from sequence provided
      ca->append(new AddMarkerAction(&seq->markers, seq->playhead, marker_name));

    }


    // push action
    olive::undo_stack.push(ca);

    // redraw UI for new markers
    update_ui(false);
    panel_footage_viewer->update_viewer();

  }

}

void Marker::Save(QXmlStreamWriter &stream) const
{
  stream.writeStartElement("marker");
  stream.writeAttribute("frame", QString::number(frame));
  stream.writeAttribute("name", name);
  stream.writeEndElement();
}
