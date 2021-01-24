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
#include "common/qtutils.h"
#include "common/timecodefunctions.h"
#include "node/input/media/media.h"
#include "project/item/footage/footage.h"
#include "ui/colorcoding.h"

namespace olive {

TimelineView::TimelineView(Qt::Alignment vertical_alignment, QWidget *parent) :
  TimeBasedView(parent),
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
  TimelineViewMouseEvent timeline_event = CreateMouseEvent(event);

  if (HandPress(event)
      || (!GetItemAtScenePos(timeline_event.GetFrame(), timeline_event.GetTrack().index()) && PlayheadPress(event))) {
    // Let the parent handle this
    return;
  }

  if (dragMode() != GetDefaultDragMode()) {
    // Use default behavior when hand dragging for instance
    TimeBasedView::mousePressEvent(event);
    return;
  }

  emit MousePressed(&timeline_event);
}

void TimelineView::mouseMoveEvent(QMouseEvent *event)
{
  TimelineViewMouseEvent timeline_event = CreateMouseEvent(event);

  if (HandMove(event) || PlayheadMove(event)) {
    // Let the parent handle this
    return;
  }

  if (dragMode() != GetDefaultDragMode()) {
    TimeBasedView::mouseMoveEvent(event);
    return;
  }

  emit MouseMoved(&timeline_event);
}

void TimelineView::mouseReleaseEvent(QMouseEvent *event)
{
  if (HandRelease(event) || PlayheadRelease(event)) {
    // Let the parent handle this
    return;
  }

  if (dragMode() != GetDefaultDragMode()) {
    TimeBasedView::mouseReleaseEvent(event);
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

  foreach (Track* track, connected_track_list_->GetTracks()) {
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
  if (!connected_track_list_) {
    return;
  }

  // Draw block backgrounds
  DrawBlocks(painter, false);

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

  // Draw block foregrounds
  DrawBlocks(painter, true);

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

  // Draw standard TimelineViewBase things (such as playhead)
  TimeBasedView::drawForeground(painter, rect);
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

Track::Type TimelineView::ConnectedTrackType()
{
  if (connected_track_list_) {
    return connected_track_list_->type();
  }

  return Track::kNone;
}

Stream::Type TimelineView::TrackTypeToStreamType(Track::Type track_type)
{
  switch (track_type) {
  case Track::kNone:
  case Track::kCount:
    break;
  case Track::kVideo:
    return Stream::kVideo;
  case Track::kAudio:
    return Stream::kAudio;
  case Track::kSubtitle:
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
  return TimelineCoordinate(SceneToTime(pt.x()), Track::Reference(ConnectedTrackType(), SceneToTrack(pt.y())));
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
                                Track::Reference(ConnectedTrackType(), SceneToTrack(scene_pt.y())),
                                button,
                                modifiers);
}

void TimelineView::DrawBlocks(QPainter *painter, bool foreground)
{
  qreal left_bound = horizontalScrollBar()->value();
  qreal right_bound = viewport()->width() + horizontalScrollBar()->value();

  rational start_time = SceneToTime(left_bound);
  rational end_time = SceneToTime(right_bound);

  foreach (Track* track, connected_track_list_->GetTracks()) {
    // Get first visible block in this track
    Block* block = track->NearestBlockBeforeOrAt(start_time);

    while (block) {
      if (block->type() == Block::kClip) {

        qreal block_left = qMax(left_bound, TimeToScene(block->in()));
        qreal block_right = qMin(right_bound, TimeToScene(block->out())) - 1;
        qreal block_top = GetTrackY(track->Index());
        qreal block_height = GetTrackHeight(track->Index());

        QRectF r(block_left,
                 block_top,
                 block_right - block_left,
                 block_height);

        QColor shadow_color = block->color().toQColor().darker();

        QFontMetrics fm = fontMetrics();
        int text_height = fm.height();
        int text_padding = text_height/4; // This ties into the track minimum height being 1.5
        int text_total_height = text_height + text_padding + text_padding;

        if (foreground) {
          painter->setBrush(Qt::NoBrush);

          QRectF text_rect = r.adjusted(text_padding, text_padding, -text_padding, -text_padding);
          painter->setPen(block->is_enabled() ? ColorCoding::GetUISelectorColor(block->color()) : Qt::lightGray);
          painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignTop, block->GetLabel());

          if (block->HasLinks()) {
            int text_width = qMin(qRound(text_rect.width()),
                                  QtUtils::QFontMetricsWidth(fm, block->GetLabel()));

            int underline_y = text_rect.y() + text_height;

            painter->drawLine(text_rect.x(), underline_y, text_width + text_rect.x(), underline_y);
          }

          qreal line_bottom = block_top+block_height-1;

          painter->setPen(Qt::white);
          painter->drawLine(block_left, block_top, block_right, block_top);
          painter->drawLine(block_left, block_top, block_left, line_bottom);

          painter->setPen(shadow_color);
          painter->drawLine(block_left, line_bottom, block_right, line_bottom);
          painter->drawLine(block_right, line_bottom, block_right, block_top);
        } else {
          painter->setPen(Qt::NoPen);
          painter->setBrush(block->is_enabled() ? block->brush(block_top, block_top + block_height) : Qt::gray);
          painter->drawRect(r);

          // Draw waveform
          QRect waveform_rect = r.adjusted(0, text_total_height, 0, 0).toRect();
          painter->setPen(shadow_color);
          AudioVisualWaveform::DrawWaveform(painter, waveform_rect, this->GetScale(), track->waveform(), SceneToTime(block_left));
        }

      }

      if (block->out() >= end_time) {
        // Rest of the clips are offscreen, can break loop now
        break;
      }

      block = block->next();
    }
  }
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
    return Track::GetDefaultTrackHeightInPixels();
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
  connected_track_list_ = list;
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

Block *TimelineView::GetItemAtScenePos(const rational &time, int track_index) const
{
  if (connected_track_list_) {
    Track* track = connected_track_list_->GetTrackAt(track_index);

    if (track) {
      foreach (Block* b, track->Blocks()) {
        if (b->in() <= time && b->out() > time) {
          return b;
        }
      }
    }
  }

  return nullptr;
}

void TimelineView::UserSetTime(const int64_t &time)
{
  SetTime(time);
  emit TimeChanged(time);
}

}
