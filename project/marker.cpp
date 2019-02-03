#include "marker.h"

#include "io/config.h"
#include "project/undo.h"
#include "mainwindow.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "panels/panels.h"

#include <QInputDialog>
#include <QCoreApplication>

void draw_marker(QPainter &p, int x, int y, int bottom, bool selected, bool flipped) {
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

void set_marker_internal(Sequence* seq, const QVector<int>& clips) {
    // if clips is empty, the marker is being added to the sequence

    // add_marker is used to determine whether we're adding a marker, depending on whether the user input a marker name
    // however if (config.set_name_with_marker) is true, we don't need a marker name so we just add
    bool add_marker = !config.set_name_with_marker;

    QString marker_name;

    // if (config.set_name_with_marker) is false (set above), ask for a marker name
    if (!add_marker) {
        QInputDialog d(mainWindow);
        d.setWindowTitle(QCoreApplication::translate("Marker", "Set Marker"));
        d.setLabelText(clips.size() > 0
                       ? QCoreApplication::translate("Marker", "Set clip marker name:")
                       : QCoreApplication::translate("Marker", "Set sequence marker name:"));
        d.setInputMode(QInputDialog::TextInput);
        add_marker = (d.exec() == QDialog::Accepted);
        marker_name = d.textValue();
    }

    // if we've decided to add a marker
    if (add_marker) {

        ComboAction* ca = new ComboAction();

        if (clips.size() > 0) {

            // add a marker action for each clip
            foreach (int i, clips) {
                Clip* c = seq->clips.at(i);
                ca->append(new AddMarkerAction(false,
                                               c,
                                               seq->playhead - c->timeline_in + c->clip_in,
                                               marker_name));
            }

        } else {

            // if no clips are selected, we're adding a marker to the sequence
            ca->append(new AddMarkerAction(true, seq, seq->playhead, marker_name));

        }


        // push action
        undo_stack.push(ca);

        // redraw UI for new markers
        update_ui(false);

    }
}

void set_marker_internal(Sequence* seq) {
    // create empty clip array
    QVector<int> clips;

    set_marker_internal(seq, clips);
}
