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
#include "node/project/footage/footage.h"
#include "ui/colorcoding.h"

namespace olive {

#define super TimeBasedView

TimelineView::TimelineView(Qt::Alignment vertical_alignment, QWidget *parent) :
  super(parent),
  selections_(nullptr),
  ghosts_(nullptr),
  show_beam_cursor_(false),
  connected_track_list_(nullptr),
  show_waveforms_(true),
  transition_overlay_out_(nullptr),
  transition_overlay_in_(nullptr)
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
      || (!GetItemAtScenePos(timeline_event.GetFrame(), timeline_event.GetTrack().index()) && Core::instance()->tool() != Tool::kAdd && PlayheadPress(event))) {
    // Let the parent handle this
    return;
  }

  if (dragMode() != GetDefaultDragMode()) {
    // Use default behavior when hand dragging for instance
    super::mousePressEvent(event);
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
    super::mouseMoveEvent(event);
    return;
  }

  if (event->buttons() == Qt::NoButton) {
    Block* b = GetItemAtScenePos(timeline_event.GetFrame(), timeline_event.GetTrack().index());
    if (b) {
      setToolTip(tr("In: %1\nOut: %2\nDuration: %3").arg(
                   Timecode::time_to_timecode(b->in(), timebase(), Core::instance()->GetTimecodeDisplay()),
                   Timecode::time_to_timecode(b->out(), timebase(), Core::instance()->GetTimecodeDisplay()),
                   Timecode::time_to_timecode(b->length(), timebase(), Core::instance()->GetTimecodeDisplay())
      ));
    } else {
      setToolTip(QString());
    }
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
    super::mouseReleaseEvent(event);
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
  if (WheelEventIsAZoomEvent(event)) {
    super::wheelEvent(event);
  } else {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))

    QPoint angle_delta = event->angleDelta();

    if (Config::Current()[QStringLiteral("InvertTimelineScrollAxes")].toBool() // Check if config is set to invert timeline axes
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

    super::wheelEvent(&e);
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
    foreach (TimelineViewGhostItem* ghost, (*ghosts_)) {
      if (ghost->GetTrack().type() == connected_track_list_->type()
          && !ghost->IsInvisible()) {
        int track_index = ghost->GetAdjustedTrack().index();

        Block *attached = Node::ValueToPtr<Block>(ghost->GetData(TimelineViewGhostItem::kAttachedBlock));

        if (attached && Config::Current()[QStringLiteral("ShowClipWhileDragging")].toBool()) {
          int adj_track = ghost->GetAdjustedTrack().index();
          qreal track_top = GetTrackY(adj_track);
          qreal track_height = GetTrackHeight(adj_track);

          qreal old_opacity = painter->opacity();
          painter->setOpacity(0.5);

          rational in = ghost->GetAdjustedIn(), out = ghost->GetAdjustedOut();
          DrawBlock(painter, false, attached, track_top, track_height, in, out);
          DrawBlock(painter, true, attached, track_top, track_height, in, out);

          painter->setOpacity(old_opacity);
        }

        painter->setPen(QPen(Qt::yellow, 2));
        painter->setBrush(Qt::NoBrush);

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
  super::drawForeground(painter, rect);
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
  case Tool::kTrackSelect:
    setCursor(Qt::SizeHorCursor); // FIXME: Not the ideal cursor
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


  rational start_time = SceneToTime(GetTimelineLeftBound());
  rational end_time = SceneToTime(GetTimelineRightBound());

  foreach (Track* track, connected_track_list_->GetTracks()) {
    // Get first visible block in this track
    Block* block = track->NearestBlockBeforeOrAt(start_time);

    qreal track_top = GetTrackY(track->Index());
    qreal track_height = GetTrackHeight(track->Index());

    while (block) {
      DrawBlock(painter, foreground, block, track_top, track_height);

      if (block->out() >= end_time) {
        // Rest of the clips are offscreen, can break loop now
        break;
      }

      block = block->next();
    }
  }
}

void TimelineView::DrawBlock(QPainter *painter, bool foreground, Block *block, qreal block_top, qreal block_height, const rational &in, const rational &out)
{
  if (dynamic_cast<ClipBlock*>(block) || dynamic_cast<TransitionBlock*>(block)) {

    qreal block_in = TimeToScene(in);

    qreal block_left = qMax(GetTimelineLeftBound(), block_in);
    qreal block_right = qMin(GetTimelineRightBound(), TimeToScene(out)) - 1;

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

      QString using_label = block->GetLabel().isEmpty() ? block->Name() : block->GetLabel();

      QRectF text_rect = r.adjusted(text_padding, text_padding, -text_padding, -text_padding);
      painter->setPen(block->is_enabled() ? ColorCoding::GetUISelectorColor(block->color()) : Qt::lightGray);
      painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignTop, using_label);

      if (block->HasLinks()) {
        int text_width = qMin(qRound(text_rect.width()),
                              QtUtils::QFontMetricsWidth(fm, using_label));

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

      if (ClipBlock *clip = dynamic_cast<ClipBlock*>(block)) {
        // Draw waveform
        if (show_waveforms_) {
          QRect waveform_rect = r.adjusted(0, text_total_height, 0, 0).toRect();
          painter->setPen(shadow_color);
          AudioVisualWaveform::DrawWaveform(painter, waveform_rect, this->GetScale(), clip->waveform(),
                                            SceneToTime(block_left - block_in, GetScale(), connected_track_list_->parent()->GetAudioParams().sample_rate_as_time_base()));
        }

        // Draw zebra stripes
        if (clip->connected_viewer() && !clip->connected_viewer()->GetLength().isNull()) {
          if (clip->media_in() < 0) {
            // Draw stripes for sections of clip < 0
            qreal zebra_right = TimeToScene(-clip->media_in());
            if (zebra_right > GetTimelineLeftBound()) {
              DrawZebraStripes(painter, QRectF(block_left, block_top, zebra_right, block_height));
            }
          }

          if (clip->length() + clip->media_in() > clip->connected_viewer()->GetLength()) {
            // Draw stripes for sections for clip > clip length
            qreal zebra_left = TimeToScene(clip->out() - (clip->media_in() + clip->length() - clip->connected_viewer()->GetLength()));
            if (zebra_left < GetTimelineRightBound()) {
              DrawZebraStripes(painter, QRectF(zebra_left, block_top, block_right - zebra_left, block_height));
            }
          }
        }
      }

      // For transitions, show lines representing a transition
      if (TransitionBlock* transition = dynamic_cast<TransitionBlock*>(block)) {
        QVector<QLineF> lines;

        if (transition->connected_in_block()) {
          lines.append(QLineF(r.bottomLeft(), r.topRight()));
        }

        if (transition->connected_out_block()) {
          lines.append(QLineF(r.topLeft(), r.bottomRight()));
        }

        painter->setPen(shadow_color);
        painter->drawLines(lines);
      }

      if (transition_overlay_out_ == block || transition_overlay_in_ == block) {
        QRectF transition_overlay_rect = r;

        qreal transition_overlay_width = TimeToScene(block->length()) * 0.5;
        if (transition_overlay_out_ && transition_overlay_in_) {
          // This is a dual transition, use the smallest width
          Block *other_block = (transition_overlay_out_ == block) ? transition_overlay_in_ : transition_overlay_out_;

          qreal other_width = TimeToScene(other_block->length()) * 0.5;

          transition_overlay_width = qMin(transition_overlay_width, other_width);
        }

        if (transition_overlay_out_ == block) {
          transition_overlay_rect.setLeft(transition_overlay_rect.right() - transition_overlay_width);
        } else {
          transition_overlay_rect.setRight(transition_overlay_rect.left() + transition_overlay_width);
        }

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 64));

        painter->drawRect(transition_overlay_rect);
      }
    }

  }
}

void TimelineView::DrawZebraStripes(QPainter *painter, const QRectF &r)
{
  int zebra_interval = fontMetrics().height();

  painter->setPen(QPen(QColor(0, 0, 0, 128), zebra_interval/4));
  painter->setBrush(Qt::NoBrush);

  QVector<QLineF> lines;
  lines.reserve(qCeil(r.width() / zebra_interval));

  qreal left = r.left() - r.height();
  qreal right = r.right() + r.height();

  for (qreal i=left; i<right; i+=zebra_interval) {
    lines.append(QLineF(i, r.top(), i - r.height(), r.bottom()));
  }

  painter->setClipRect(r);
  painter->drawLines(lines);
  painter->setClipping(false);
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

qreal TimelineView::GetTimelineLeftBound() const
{
  return horizontalScrollBar()->value();
}

qreal TimelineView::GetTimelineRightBound() const
{
  return GetTimelineLeftBound() + viewport()->width();
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
  if (connected_track_list_) {
    disconnect(connected_track_list_, &TrackList::TrackListChanged, this, &TimelineView::TrackListChanged);
  }

  connected_track_list_ = list;

  if (connected_track_list_) {
    connect(connected_track_list_, &TrackList::TrackListChanged, this, &TimelineView::TrackListChanged);
  }
}

void TimelineView::SetBeamCursor(const TimelineCoordinate &coord)
{
  if (!connected_track_list_) {
    return;
  }

  bool update_required = coord.GetTrack().type() == connected_track_list_->type()
      || cursor_coord_.GetTrack().type() == connected_track_list_->type();

  show_beam_cursor_ = true;
  cursor_coord_ = coord;

  if (update_required) {
    viewport()->update();
  }
}

void TimelineView::SetTransitionOverlay(ClipBlock *out, ClipBlock *in)
{
  if (transition_overlay_out_ != out || transition_overlay_in_ != in) {
    Track::Type type = Track::kNone;

    if (out) {
      type = out->track()->type();
    } else if (in) {
      type = in->track()->type();
    }

    if (type == this->connected_track_list_->type()) {
      transition_overlay_out_ = out;
      transition_overlay_in_ = in;
    } else {
      transition_overlay_out_ = nullptr;
      transition_overlay_in_ = nullptr;
    }

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

void TimelineView::TrackListChanged()
{
  UpdateSceneRect();
  viewport()->update();
}

}
