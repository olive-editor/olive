/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "timelineview.h"

#include <QDebug>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>
#include <QtMath>
#include <QPen>

#include "config/config.h"
#include "common/flipmodifiers.h"
#include "common/timecodefunctions.h"
#include "node/input/media/media.h"
#include "project/item/footage/footage.h"

namespace olive {

TimelineView::TimelineView(Qt::Alignment vertical_alignment, QWidget *parent) :
  TimelineViewBase(parent),
  selections_(nullptr),
  ghosts_(nullptr),
  show_beam_cursor_(false),
  connected_track_list_(nullptr)
{
  Q_ASSERT(vertical_alignment == Qt::AlignTop || vertical_alignment == Qt::AlignBottom);
  setAlignment(Qt::AlignLeft | vertical_alignment);

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setBackgroundRole(QPalette::Window);
  setContextMenuPolicy(Qt::CustomContextMenu);
  viewport()->setMouseTracking(true);
}

void TimelineView::mousePressEvent(QMouseEvent *event)
{
  if (HandPress(event) || PlayheadPress(event)) {
    // Let the parent handle this
    return;
  }

  if (dragMode() != GetDefaultDragMode()) {
    TimelineViewBase::mousePressEvent(event);
    return;
  }

  TimelineViewMouseEvent timeline_event = CreateMouseEvent(event);

  emit MousePressed(&timeline_event);
}

void TimelineView::mouseMoveEvent(QMouseEvent *event)
{
  if (HandMove(event) || PlayheadMove(event)) {
    // Let the parent handle this
    return;
  }

  if (dragMode() != GetDefaultDragMode()) {
    TimelineViewBase::mouseMoveEvent(event);
    return;
  }

  TimelineViewMouseEvent timeline_event = CreateMouseEvent(event);

  emit MouseMoved(&timeline_event);
}

void TimelineView::mouseReleaseEvent(QMouseEvent *event)
{
  if (HandRelease(event) || PlayheadRelease(event)) {
    // Let the parent handle this
    return;
  }

  if (dragMode() != GetDefaultDragMode()) {
    TimelineViewBase::mouseReleaseEvent(event);
    return;
  }

  TimelineViewMouseEvent timeline_event = CreateMouseEvent(event);

  emit MouseReleased(&timeline_event);
}

void TimelineView::mouseDoubleClickEvent(QMouseEvent *event)
{
  TimelineViewMouseEvent timeline_event = CreateMouseEvent(event);

  emit MouseDoubleClicked(&timeline_event);
}

void TimelineView::wheelEvent(QWheelEvent *event)
{
  if (HandleZoomFromScroll(event)) {
    return;
  } else {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))

    QPoint angle_delta = event->angleDelta();

    if (Config::Current()["InvertTimelineScrollAxes"].toBool() // Check if config is set to invert timeline axes
        && event->source() != Qt::MouseEventSynthesizedBySystem) { // Never flip axes on Apple trackpads though
      angle_delta = QPoint(angle_delta.y(), angle_delta.x());
    }

    QWheelEvent e(
      #if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
          event->position(),
          event->globalPosition(),
      #else
          event->pos(),
          event->globalPos(),
      #endif
          event->pixelDelta(),
          angle_delta,
          event->buttons(),
          event->modifiers(),
          event->phase(),
          event->inverted(),
          event->source()
          );

#else

    Qt::Orientation orientation = event->orientation();

    if (Config::Current()["InvertTimelineScrollAxes"].toBool()) {
      orientation = (orientation == Qt::Horizontal) ? Qt::Vertical : Qt::Horizontal;
    }

    QWheelEvent e(
          event->pos(),
          event->globalPos(),
          event->pixelDelta(),
          event->angleDelta(),
          event->delta(),
          orientation,
          event->buttons(),
          event->modifiers()
          );
#endif

    QGraphicsView::wheelEvent(&e);
  }
}

void TimelineView::dragEnterEvent(QDragEnterEvent *event)
{
  TimelineViewMouseEvent timeline_event = CreateMouseEvent(event->pos(), Qt::NoButton, event->keyboardModifiers());

  timeline_event.SetMimeData(event->mimeData());
  timeline_event.SetEvent(event);

  emit DragEntered(&timeline_event);
}

void TimelineView::dragMoveEvent(QDragMoveEvent *event)
{
  TimelineViewMouseEvent timeline_event = CreateMouseEvent(event->pos(), Qt::NoButton, event->keyboardModifiers());

  timeline_event.SetMimeData(event->mimeData());
  timeline_event.SetEvent(event);

  emit DragMoved(&timeline_event);
}

void TimelineView::dragLeaveEvent(QDragLeaveEvent *event)
{
  emit DragLeft(event);
}

void TimelineView::dropEvent(QDropEvent *event)
{
  TimelineViewMouseEvent timeline_event = CreateMouseEvent(event->pos(), Qt::NoButton, event->keyboardModifiers());

  timeline_event.SetMimeData(event->mimeData());
  timeline_event.SetEvent(event);

  emit DragDropped(&timeline_event);
}

void TimelineView::drawBackground(QPainter *painter, const QRectF &rect)
{
  if (!connected_track_list_) {
    return;
  }

  painter->setPen(palette().base().color());

  int line_y = 0;

  foreach (TrackOutput* track, connected_track_list_->GetTracks()) {
    line_y += track->GetTrackHeightInPixels();

    // One px gap between tracks
    line_y++;

    int this_line_y;

    if (alignment() & Qt::AlignTop) {
      this_line_y = line_y;
    } else {
      this_line_y = -line_y;
    }

    painter->drawLine(qRound(rect.left()), this_line_y, qRound(rect.right()), this_line_y);
  }
}

void TimelineView::drawForeground(QPainter *painter, const QRectF &rect)
{
  TimelineViewBase::drawForeground(painter, rect);

  // Draw selections
  if (selections_ && !selections_->isEmpty()) {
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 64));

    for (auto it=selections_->cbegin(); it!=selections_->cend(); it++) {
      if (it.key().type() == connected_track_list_->type()) {
        int track_index = it.key().index();

        foreach (const TimeRange& range, it.value()) {
          painter->drawRect(TimeToScene(range.in()),
                            GetTrackY(track_index),
                            TimeToScene(range.length()),
                            GetTrackHeight(track_index));
        }
      }
    }
  }

  // Draw ghosts
  if (ghosts_ && !ghosts_->isEmpty()) {
    painter->setPen(QPen(Qt::yellow, 2));
    painter->setBrush(Qt::NoBrush);

    foreach (TimelineViewGhostItem* ghost, (*ghosts_)) {
      if (ghost->GetTrack().type() == connected_track_list_->type()
          && !ghost->IsInvisible()) {
        int track_index = ghost->GetAdjustedTrack().index();

        painter->drawRect(TimeToScene(ghost->GetAdjustedIn()),
                          GetTrackY(track_index),
                          TimeToScene(ghost->GetAdjustedLength()),
                          GetTrackHeight(track_index));
      }
    }
  }

  // Draw beam cursor
  if (show_beam_cursor_
      && connected_track_list_
      && cursor_coord_.GetTrack().type() == connected_track_list_->type()) {
    painter->setPen(Qt::gray);

    double cursor_x = TimeToScene(cursor_coord_.GetFrame());
    int track_index = cursor_coord_.GetTrack().index();
    int track_y = GetTrackY(track_index);

    painter->drawLine(cursor_x,
                      track_y,
                      cursor_x,
                      track_y + GetTrackHeight(track_index));
  }
}

void TimelineView::ToolChangedEvent(Tool::Item tool)
{
  switch (tool) {
  case Tool::kRazor:
    setCursor(Qt::SplitHCursor);
    break;
  case Tool::kEdit:
    setCursor(Qt::IBeamCursor);
    break;
  case Tool::kAdd:
  case Tool::kTransition:
  case Tool::kZoom:
    setCursor(Qt::CrossCursor);
    break;
  default:
    unsetCursor();
  }

  // Hide/show cursor if necessary
  if (show_beam_cursor_) {
    show_beam_cursor_ = false;
    viewport()->update();
  }
}

void TimelineView::SceneRectUpdateEvent(QRectF &rect)
{
  if (alignment() & Qt::AlignTop) {
    rect.setTop(0);
    rect.setBottom(GetHeightOfAllTracks() + height() / 2);
  } else if (alignment() & Qt::AlignBottom) {
    rect.setBottom(0);
    rect.setTop(GetHeightOfAllTracks() - height() / 2);
  }
}

Timeline::TrackType TimelineView::ConnectedTrackType()
{
  if (connected_track_list_) {
    return connected_track_list_->type();
  }

  return Timeline::kTrackTypeNone;
}

Stream::Type TimelineView::TrackTypeToStreamType(Timeline::TrackType track_type)
{
  switch (track_type) {
  case Timeline::kTrackTypeNone:
  case Timeline::kTrackTypeCount:
    break;
  case Timeline::kTrackTypeVideo:
    return Stream::kVideo;
  case Timeline::kTrackTypeAudio:
    return Stream::kAudio;
  case Timeline::kTrackTypeSubtitle:
    return Stream::kSubtitle;
  }

  return Stream::kUnknown;
}

TimelineCoordinate TimelineView::ScreenToCoordinate(const QPoint& pt)
{
  return SceneToCoordinate(mapToScene(pt));
}

TimelineCoordinate TimelineView::SceneToCoordinate(const QPointF& pt)
{
  return TimelineCoordinate(SceneToTime(pt.x()), TrackReference(ConnectedTrackType(), SceneToTrack(pt.y())));
}

TimelineViewMouseEvent TimelineView::CreateMouseEvent(QMouseEvent *event)
{
  return CreateMouseEvent(event->pos(), event->button(), event->modifiers());
}

TimelineViewMouseEvent TimelineView::CreateMouseEvent(const QPoint& pos, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
  QPointF scene_pt = mapToScene(pos);

  return TimelineViewMouseEvent(scene_pt.x(),
                                GetScale(),
                                timebase(),
                                TrackReference(ConnectedTrackType(), SceneToTrack(scene_pt.y())),
                                button,
                                modifiers);
}

int TimelineView::GetHeightOfAllTracks() const
{
  if (connected_track_list_) {
    if (alignment() & Qt::AlignTop) {
      return GetTrackY(connected_track_list_->GetTrackCount());
    } else {
      return GetTrackY(connected_track_list_->GetTrackCount() - 1);
    }
  } else {
    return 0;
  }
}

int TimelineView::GetTrackY(int track_index) const
{
  if (!connected_track_list_ || !connected_track_list_->GetTrackCount()) {
    return 0;
  }

  int y = 0;

  if (alignment() & Qt::AlignBottom) {
    track_index++;
  }

  for (int i=0;i<track_index;i++) {
    y += GetTrackHeight(i);

    // One px line between each track
    y++;
  }

  if (alignment() & Qt::AlignBottom) {
    y = -y + 1;
  }

  return y;
}

int TimelineView::GetTrackHeight(int track_index) const
{
  if (!connected_track_list_ || track_index >= connected_track_list_->GetTrackCount()) {
    return TrackOutput::GetDefaultTrackHeightInPixels();
  }

  return connected_track_list_->GetTrackAt(track_index)->GetTrackHeightInPixels();
}

QPoint TimelineView::GetScrollCoordinates() const
{
  return QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value());
}

void TimelineView::SetScrollCoordinates(const QPoint &pt)
{
  horizontalScrollBar()->setValue(pt.x());
  verticalScrollBar()->setValue(pt.y());
}

void TimelineView::ConnectTrackList(TrackList *list)
{
  if (connected_track_list_) {
    disconnect(connected_track_list_, SIGNAL(TrackHeightChanged(int, int)), viewport(), SLOT(update()));
  }

  connected_track_list_ = list;

  if (connected_track_list_) {
    connect(connected_track_list_, SIGNAL(TrackHeightChanged(int, int)), viewport(), SLOT(update()));
  }
}

void TimelineView::SetBeamCursor(const TimelineCoordinate &coord)
{
  bool update_required = coord.GetTrack().type() == connected_track_list_->type()
                          || cursor_coord_.GetTrack().type() == connected_track_list_->type();

  show_beam_cursor_ = true;
  cursor_coord_ = coord;

  if (update_required) {
    viewport()->update();
  }
}

int TimelineView::SceneToTrack(double y)
{
  int track = -1;
  int heights = 0;

  if (alignment() & Qt::AlignBottom) {
    y = -y;
  }

  do {
    track++;
    heights += GetTrackHeight(track);
  } while (y > heights);

  return track;
}

void TimelineView::UserSetTime(const int64_t &time)
{
  SetTime(time);
  emit TimeChanged(time);
}

}
