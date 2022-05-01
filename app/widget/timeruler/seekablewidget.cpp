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

#include "seekablewidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

#include "common/qtutils.h"
#include "core.h"
#include "widget/timebased/timebasedwidget.h"

namespace olive {

#define super TimeBasedView

SeekableWidget::SeekableWidget(QWidget* parent) :
  super(parent),
  timeline_points_(nullptr),
  dragging_(false),
  selection_manager_(this)
{
  QFontMetrics fm = fontMetrics();

  text_height_ = fm.height();

  // Set width of playhead marker
  playhead_width_ = QtUtils::QFontMetricsWidth(fm, "H");

  setContextMenuPolicy(Qt::CustomContextMenu);
  setFocusPolicy(Qt::ClickFocus);
}

void SeekableWidget::ConnectTimelinePoints(TimelinePoints *points)
{
  if (timeline_points_) {
    selection_manager_.ClearSelection();

    disconnect(timeline_points_->workarea(), &TimelineWorkArea::RangeChanged, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    disconnect(timeline_points_->workarea(), &TimelineWorkArea::EnabledChanged, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    disconnect(timeline_points_->markers(), &TimelineMarkerList::MarkerAdded, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    disconnect(timeline_points_->markers(), &TimelineMarkerList::MarkerRemoved, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
  }

  timeline_points_ = points;

  if (timeline_points_) {
    connect(timeline_points_->workarea(), &TimelineWorkArea::RangeChanged, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    connect(timeline_points_->workarea(), &TimelineWorkArea::EnabledChanged, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    connect(timeline_points_->markers(), &TimelineMarkerList::MarkerAdded, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    connect(timeline_points_->markers(), &TimelineMarkerList::MarkerRemoved, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
  }

  viewport()->update();
}

void SeekableWidget::DeleteSelected()
{
  MultiUndoCommand* command = new MultiUndoCommand();

  foreach (TimelineMarker *marker, selection_manager_.GetSelectedObjects()) {
    command->add_child(new MarkerRemoveCommand(marker));
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void SeekableWidget::CopySelected(bool cut)
{
  qDebug() << "COPY IS STUB";
  //CopyMarkersToClipboard(selection_manager_.GetSelectedObjects());

  if (cut) {
    DeleteSelected();
  }
}

void SeekableWidget::PasteMarkers(bool insert, rational insert_time)
{
  //MultiUndoCommand *command = new MultiUndoCommand();
  //PasteMarkersFromClipboard(timeline_points()->markers(), command, insert_time);
  qDebug() << "PASTE IS STUB";
}

void SeekableWidget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton) {
    SeekToScenePoint(mapToScene(event->pos()).x());
    dragging_ = true;

    DeselectAllMarkers();
  }
}

void SeekableWidget::mouseMoveEvent(QMouseEvent *event)
{
  if (event->buttons() & Qt::LeftButton) {
    SeekToScenePoint(mapToScene(event->pos()).x());
  }
}

void SeekableWidget::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event)

  if (GetSnapService()) {
    GetSnapService()->HideSnaps();
  }

  dragging_ = false;
}

void SeekableWidget::focusOutEvent(QFocusEvent *event)
{
  super::focusOutEvent(event);

  // Deselect everything when we lose focus
  DeselectAllMarkers();
}

TimelinePoints *SeekableWidget::timeline_points() const
{
  return timeline_points_;
}

void SeekableWidget::DeselectAllMarkers()
{
  selection_manager_.ClearSelection();

  viewport()->update();
}

void SeekableWidget::SetMarkerColor(int c)
{
  MultiUndoCommand *command = new MultiUndoCommand();

  foreach(TimelineMarker* marker, selection_manager_.GetSelectedObjects()) {
    command->add_child(new MarkerChangeColorCommand(marker, c));
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void SeekableWidget::SeekToScenePoint(qreal scene)
{
  if (timebase().isNull()) {
    return;
  }

  rational playhead_time = SceneToTime(scene);

  if (Core::instance()->snapping() && GetSnapService()) {
    rational movement;

    GetSnapService()->SnapPoint({playhead_time},
                                 &movement,
                                 SnapService::kSnapAll & ~SnapService::kSnapToPlayhead);

    playhead_time += movement;
  }

  if (playhead_time != GetTime()) {
    SetTime(playhead_time);

    emit TimeChanged(playhead_time);
  }
}

void SeekableWidget::DrawTimelinePoints(QPainter* p, int marker_bottom)
{
  if (!timeline_points()) {
    return;
  }

  // Draw in/out workarea
  if (timeline_points()->workarea()->enabled()) {
    int workarea_left = qMax(0.0, TimeToScene(timeline_points()->workarea()->in()));
    int workarea_right;

    if (timeline_points()->workarea()->out() == TimelineWorkArea::kResetOut) {
      workarea_right = width();
    } else {
      workarea_right = qMin(qreal(width()), TimeToScene(timeline_points()->workarea()->out()));
    }

    p->fillRect(workarea_left, 0, workarea_right - workarea_left, height(), palette().highlight());
  }

  // Draw markers
  if (marker_bottom > 0 && !timeline_points()->markers()->list().empty()) {

    int marker_top = marker_bottom - text_height_;

    // FIXME: Hardcoded marker colors
    p->setPen(Qt::black);
    p->setBrush(Qt::green);

    foreach (TimelineMarker* marker, timeline_points()->markers()->list()) {
      int marker_left = TimeToScene(marker->time().in());
      int marker_right = TimeToScene(marker->time().out());

      if (marker_left >= width() || marker_right < 0)  {
        continue;
      }

      if (marker->time().length() != 0) {
        // Marker range
        int rect_left = qMax(0, marker_left);
        int rect_right = qMin(width(), marker_right);

        QRect marker_rect(rect_left, marker_top, rect_right - rect_left, marker_bottom - marker_top);

        p->drawRect(marker_rect);

        if (!marker->name().isEmpty()) {
          p->drawText(marker_rect, marker->name());
        }
      }
    }
  }
}

void SeekableWidget::DrawPlayhead(QPainter *p, int x, int y)
{
  int half_width = playhead_width_ / 2;

  if (x + half_width < 0 || x - half_width > width()) {
    return;
  }

  p->setRenderHint(QPainter::Antialiasing);

  int half_text_height = text_height() / 3;

  QPoint points[] = {
    QPoint(x, y),
    QPoint(x - half_width, y - half_text_height),
    QPoint(x - half_width, y - text_height()),
    QPoint(x + 1 + half_width, y - text_height()),
    QPoint(x + 1 + half_width, y - half_text_height),
    QPoint(x + 1, y),
  };

  p->drawPolygon(points, 6);

  p->setRenderHint(QPainter::Antialiasing, false);
}

}
