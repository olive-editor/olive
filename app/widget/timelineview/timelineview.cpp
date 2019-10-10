/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "common/flipmodifiers.h"
#include "common/timecodefunctions.h"
#include "core.h"
#include "node/input/media/media.h"
#include "project/item/footage/footage.h"
#include "tool/tool.h"

TimelineView::TimelineView(Qt::Alignment vertical_alignment, QWidget *parent) :
  QGraphicsView(parent),
  timeline_node_(nullptr),
  playhead_(0),
  use_tracklist_length_directly_(true)
{
  Q_ASSERT(vertical_alignment == Qt::AlignTop || vertical_alignment == Qt::AlignBottom);
  setAlignment(Qt::AlignLeft | vertical_alignment);

  setScene(&scene_);
  setDragMode(RubberBandDrag);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setBackgroundRole(QPalette::Window);

  // Create tools
  tools_.resize(olive::tool::kCount);
  tools_.fill(nullptr);

  tools_.replace(olive::tool::kPointer, std::make_shared<PointerTool>(this));
  // tools_.replace(olive::tool::kEdit, new PointerTool(this)); FIXME: Implement
  tools_.replace(olive::tool::kRipple, std::make_shared<RippleTool>(this));
  tools_.replace(olive::tool::kRolling, std::make_shared<RollingTool>(this));
  tools_.replace(olive::tool::kRazor, std::make_shared<RazorTool>(this));
  tools_.replace(olive::tool::kSlip, std::make_shared<SlipTool>(this));
  tools_.replace(olive::tool::kSlide, std::make_shared<SlideTool>(this));
  tools_.replace(olive::tool::kHand, std::make_shared<HandTool>(this));
  tools_.replace(olive::tool::kZoom, std::make_shared<ZoomTool>(this));
  //tools_.replace(olive::tool::kTransition, new (this)); FIXME: Implement
  //tools_.replace(olive::tool::kRecord, new PointerTool(this)); FIXME: Implement
  //tools_.replace(olive::tool::kAdd, new PointerTool(this)); FIXME: Implement

  import_tool_ = std::make_shared<ImportTool>(this);

  connect(&scene_, SIGNAL(changed(const QList<QRectF>&)), this, SLOT(UpdateSceneRect()));

  // Create end item
  end_item_ = new TimelineViewEndItem();
  scene_.addItem(end_item_);

  // Set default scale
  SetScale(1.0);
}

void TimelineView::AddBlock(Block *block, int track)
{
  switch (block->type()) {
  case Block::kClip:
  case Block::kGap:
  {
    TimelineViewBlockItem* item = new TimelineViewBlockItem();

    // Set up clip with view parameters (clip item will automatically size its rect accordingly)
    item->SetBlock(block);
    item->SetY(GetTrackY(track));
    item->SetHeight(GetTrackHeight(track));
    item->SetScale(scale_);
    item->SetTrack(track);

    // Add to list of clip items that can be iterated through
    block_items_.insert(block, item);

    // Add item to graphics scene
    scene_.addItem(item);

    connect(block, SIGNAL(Refreshed()), this, SLOT(BlockChanged()));
    break;
  }
  case Block::kEnd:
    // Do nothing
    break;
  }
}

void TimelineView::RemoveBlock(Block *block)
{
  delete block_items_[block];

  block_items_.remove(block);
}

void TimelineView::AddTrack(TrackOutput *track)
{
  foreach (Block* b, track->Blocks()) {
    AddBlock(b, track->Index());
  }
}

void TimelineView::RemoveTrack(TrackOutput *track)
{
  foreach (Block* b, track->Blocks()) {
    RemoveBlock(b);
  }
}

void TimelineView::SetScale(const double &scale)
{
  scale_ = scale;

  QMapIterator<Block*, TimelineViewBlockItem*> iterator(block_items_);

  while (iterator.hasNext()) {
    iterator.next();

    if (iterator.value() != nullptr) {
      iterator.value()->SetScale(scale_);
    }
  }

  foreach (TimelineViewGhostItem* ghost, ghost_items_) {
    ghost->SetScale(scale_);
  }

  // Force redraw for playhead
  viewport()->update();

  end_item_->SetScale(scale_);
}

void TimelineView::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;
  timebase_dbl_ = timebase_.toDouble();

  // Timebase influences position/visibility of playhead
  viewport()->update();
}

void TimelineView::Clear()
{
  QMapIterator<Block*, TimelineViewBlockItem*> iterator(block_items_);

  while (iterator.hasNext()) {
    iterator.next();

    if (iterator.value() != nullptr) {
      delete iterator.value();
    }
  }

  block_items_.clear();
}

void TimelineView::ConnectTimelineNode(TrackList *node)
{
  if (timeline_node_ != nullptr) {
    disconnect(timeline_node_, SIGNAL(TimebaseChanged(const rational&)), this, SIGNAL(TimebaseChanged(const rational&)));
    disconnect(timeline_node_, SIGNAL(TimebaseChanged(const rational&)), this, SLOT(SetTimebase(const rational&)));
    disconnect(timeline_node_, SIGNAL(TimelineCleared()), this, SLOT(Clear()));
    disconnect(timeline_node_, SIGNAL(BlockAdded(Block*, int)), this, SLOT(AddBlock(Block*, int)));
    disconnect(timeline_node_, SIGNAL(BlockRemoved(Block*)), this, SLOT(RemoveBlock(Block*)));
    disconnect(timeline_node_, SIGNAL(TrackAdded(TrackOutput*)), this, SLOT(AddTrack(TrackOutput*)));
    disconnect(timeline_node_, SIGNAL(TrackRemoved(TrackOutput*)), this, SLOT(RemoveTrack(TrackOutput*)));
    disconnect(timeline_node_, SIGNAL(LengthChanged(const rational&)), this, SLOT(UpdateEndTimeFromTrackList(const rational&)));

    Clear();
  }

  timeline_node_ = node;

  if (timeline_node_ != nullptr) {
    SetTimebase(timeline_node_->Timebase());
    emit TimebaseChanged(timebase_);

    connect(timeline_node_, SIGNAL(TimebaseChanged(const rational&)), this, SIGNAL(TimebaseChanged(const rational&)));
    connect(timeline_node_, SIGNAL(TimebaseChanged(const rational&)), this, SLOT(SetTimebase(const rational&)));
    connect(timeline_node_, SIGNAL(TimelineCleared()), this, SLOT(Clear()));
    connect(timeline_node_, SIGNAL(BlockAdded(Block*, int)), this, SLOT(AddBlock(Block*, int)));
    connect(timeline_node_, SIGNAL(BlockRemoved(Block*)), this, SLOT(RemoveBlock(Block*)));
    connect(timeline_node_, SIGNAL(TrackAdded(TrackOutput*)), this, SLOT(AddTrack(TrackOutput*)));
    connect(timeline_node_, SIGNAL(TrackRemoved(TrackOutput*)), this, SLOT(RemoveTrack(TrackOutput*)));
    connect(timeline_node_, SIGNAL(LengthChanged(const rational&)), this, SLOT(UpdateEndTimeFromTrackList(const rational&)));

    foreach (TrackOutput* track, timeline_node_->Tracks()) {
      // Defer to the track to make all the block UI items necessary
      AddTrack(track);
    }
  }
}

void TimelineView::DisconnectTimelineNode()
{
  ConnectTimelineNode(nullptr);
}

void TimelineView::SelectAll()
{
  QList<QGraphicsItem*> all_items = items();

  foreach (QGraphicsItem* i, all_items) {
    i->setSelected(true);
  }
}

void TimelineView::DeselectAll()
{
  QList<QGraphicsItem*> all_items = items();

  foreach (QGraphicsItem* i, all_items) {
    i->setSelected(false);
  }
}

void TimelineView::SetUseTrackListLengthDirectly(bool use)
{
  use_tracklist_length_directly_ = use;
}

void TimelineView::SetTime(const int64_t time)
{
  playhead_ = time;

  // Force redraw for playhead
  viewport()->update();
}

void TimelineView::mousePressEvent(QMouseEvent *event)
{
  active_tool_ = GetActiveTool();

  // Cache these since we modify the event's later on
  Qt::KeyboardModifiers mods = event->modifiers();

  if (active_tool_ != nullptr) {
    setDragMode(active_tool_->drag_mode());
  }

  if (active_tool_ == nullptr || active_tool_->enable_default_behavior()) {
    // We use Shift for multiple selection while Qt uses Ctrl, we flip those modifiers here to compensate
    event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

    QGraphicsView::mousePressEvent(event);
  }

  if (timeline_node_ != nullptr && active_tool_ != nullptr) {
    TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                          mods);

    active_tool_->MousePress(&timeline_event);
  }
}

void TimelineView::mouseMoveEvent(QMouseEvent *event)
{
  // Cache these since we modify the event's later on
  Qt::KeyboardModifiers mods = event->modifiers();

  if (active_tool_ == nullptr || active_tool_->enable_default_behavior()) {
    // We use Shift for multiple selection while Qt uses Ctrl, we flip those modifiers here to compensate
    event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

    QGraphicsView::mouseMoveEvent(event);
  }

  if (timeline_node_ != nullptr && active_tool_ != nullptr) {
    TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                          mods);

    active_tool_->MouseMove(&timeline_event);
  }
}

void TimelineView::mouseReleaseEvent(QMouseEvent *event)
{
  // Cache these since we modify the event's later on
  Qt::KeyboardModifiers mods = event->modifiers();

  if (active_tool_ == nullptr || active_tool_->enable_default_behavior()) {
    // We use Shift for multiple selection while Qt uses Ctrl, we flip those modifiers here to compensate
    event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

    QGraphicsView::mouseReleaseEvent(event);
  }

  if (timeline_node_ != nullptr && active_tool_ != nullptr) {
    TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                          mods);

    active_tool_->MouseRelease(&timeline_event);
  }
}

void TimelineView::mouseDoubleClickEvent(QMouseEvent *event)
{
  // Cache these since we modify the event's later on
  Qt::KeyboardModifiers mods = event->modifiers();

  if (active_tool_ == nullptr || active_tool_->enable_default_behavior()) {
    // We use Shift for multiple selection while Qt uses Ctrl, we flip those modifiers here to compensate
    event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

    QGraphicsView::mouseDoubleClickEvent(event);
  }

  if (timeline_node_ != nullptr && active_tool_ != nullptr) {
    TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                          mods);

    active_tool_->MouseDoubleClick(&timeline_event);
  }
}

void TimelineView::dragEnterEvent(QDragEnterEvent *event)
{
  if (timeline_node_ != nullptr) {
    TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                          event->keyboardModifiers());

    timeline_event.SetMimeData(event->mimeData());
    timeline_event.SetEvent(event);

    import_tool_->DragEnter(&timeline_event);
  }
}

void TimelineView::dragMoveEvent(QDragMoveEvent *event)
{
  if (timeline_node_ != nullptr) {
    TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                          event->keyboardModifiers());

    timeline_event.SetMimeData(event->mimeData());
    timeline_event.SetEvent(event);

    import_tool_->DragMove(&timeline_event);
  }
}

void TimelineView::dragLeaveEvent(QDragLeaveEvent *event)
{
  if (timeline_node_ != nullptr) {
    import_tool_->DragLeave(event);
  }
}

void TimelineView::dropEvent(QDropEvent *event)
{
  if (timeline_node_ != nullptr) {
    TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                          event->keyboardModifiers());

    timeline_event.SetMimeData(event->mimeData());
    timeline_event.SetEvent(event);

    import_tool_->DragDrop(&timeline_event);
  }
}

void TimelineView::resizeEvent(QResizeEvent *event)
{
  QGraphicsView::resizeEvent(event);

  UpdateSceneRect();
}

void TimelineView::drawForeground(QPainter *painter, const QRectF &rect)
{
  QGraphicsView::drawForeground(painter, rect);

  if (!timebase_.isNull()) {
    double x = TimeToScreenCoord(rational(playhead_ * timebase_.numerator(), timebase_.denominator()));
    double width = TimeToScreenCoord(timebase_);

    QRectF playhead_rect(x, rect.top(), width, rect.height());

    painter->setPen(Qt::NoPen);
    painter->setBrush(playhead_style_.PlayheadHighlightColor());
    painter->drawRect(playhead_rect);

    painter->setPen(playhead_style_.PlayheadColor());
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(QLineF(playhead_rect.topLeft(), playhead_rect.bottomLeft()));
  }
}

TrackType TimelineView::ConnectedTrackType()
{
  if (timeline_node_ != nullptr) {
    return timeline_node_->TrackType();
  }

  return kTrackTypeNone;
}

Stream::Type TimelineView::TrackTypeToStreamType(TrackType track_type)
{
  switch (track_type) {
  case kTrackTypeNone:
  case kTrackTypeCount:
    break;
  case kTrackTypeVideo:
    return Stream::kVideo;
  case kTrackTypeAudio:
    return Stream::kAudio;
  case kTrackTypeSubtitle:
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
  return TimelineCoordinate(SceneToTime(pt.x()), SceneToTrack(pt.y()));
}

TimelineView::Tool *TimelineView::GetActiveTool()
{
  return tools_.at(olive::core.tool()).get();
}

int TimelineView::GetTrackY(int track_index)
{
  int y = 0;

  for (int i=0;i<track_index;i++) {
    y += GetTrackHeight(i);
  }

  if (alignment() & Qt::AlignBottom) {
    y = -y - GetTrackHeight(0);
  }

  return y;
}

int TimelineView::GetTrackHeight(int track_index)
{
  // FIXME: Make this adjustable
  Q_UNUSED(track_index)

  return fontMetrics().height() * 3;
}

void TimelineView::AddGhost(TimelineViewGhostItem *ghost)
{
  ghost->SetScale(scale_);
  ghost_items_.append(ghost);
  scene_.addItem(ghost);
}

bool TimelineView::HasGhosts()
{
  return !ghost_items_.isEmpty();
}

rational TimelineView::SceneToTime(const double &x)
{
  // Adjust screen point by scale and timebase
  qint64 scaled_x_mvmt = qRound64(x / scale_ / timebase_dbl_);

  // Return a time in the timebase
  return rational(scaled_x_mvmt * timebase_.numerator(), timebase_.denominator());
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

void TimelineView::ClearGhosts()
{
  if (!ghost_items_.isEmpty()) {
    foreach (TimelineViewGhostItem* ghost, ghost_items_) {
      delete ghost;
    }

    ghost_items_.clear();
  }
}

void TimelineView::UserSetTime(const int64_t &time)
{
  SetTime(time);
  emit TimeChanged(time);
}

rational TimelineView::GetPlayheadTime()
{
  return rational(playhead_ * timebase_.numerator(), timebase_.denominator());
}

void TimelineView::BlockChanged()
{
  TimelineViewRect* rect = block_items_[static_cast<Block*>(sender())];

  if (rect != nullptr) {
    rect->UpdateRect();
  }
}

void TimelineView::UpdateSceneRect()
{
  QRectF bounding_rect = scene_.itemsBoundingRect();

  // Ensure the scene height is always AT LEAST the height of the view
  // The scrollbar appears to have a 1px margin on the top and bottom, hence the -2
  int minimum_height = height() - horizontalScrollBar()->height() - 2;

  if (alignment() & Qt::AlignBottom) {
    // Ensure the scene left and bottom are always 0
    bounding_rect.setBottomLeft(QPointF(0, 0));

    if (bounding_rect.top() > minimum_height) {
      bounding_rect.setTop(-minimum_height);
    }
  } else {
    // Ensure the scene left and top are always 0
    bounding_rect.setTopLeft(QPointF(0, 0));

    if (bounding_rect.height() < minimum_height) {
      bounding_rect.setHeight(minimum_height);
    }
  }

  // Ensure the scene is always the full length of the timeline with a gap at the end to work with
  end_item_->SetEndPadding(width()/4);

  // If the scene is already this rect, do nothing
  if (scene_.sceneRect() == bounding_rect) {
    return;
  }

  scene_.setSceneRect(bounding_rect);
}

void TimelineView::UpdateEndTimeFromTrackList(const rational &length)
{
  if (use_tracklist_length_directly_) {
    SetEndTime(length);
  }
}

void TimelineView::SetEndTime(const rational &length)
{
  end_item_->SetEndTime(length);
}

void TimelineView::SetSiblings(TimelineView *a, TimelineView *b)
{
  if (a == b)
    return;

  if (!a->siblings_.contains(b))
    a->siblings_.append(b);

  if (!b->siblings_.contains(a))
    b->siblings_.append(a);
}

void TimelineView::SetSiblings(const QList<TimelineView *> &siblings)
{
  foreach (TimelineView* view, siblings) {
    foreach (TimelineView* sibling, siblings) {
      SetSiblings(view, sibling);
    }
  }
}
