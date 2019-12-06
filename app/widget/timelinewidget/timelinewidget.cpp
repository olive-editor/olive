#include "timelinewidget.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QtMath>

#include "core.h"
#include "common/timecodefunctions.h"
#include "tool/tool.h"

TimelineWidget::TimelineWidget(QWidget *parent) :
  QWidget(parent),
  rubberband_(QRubberBand::Rectangle, this),
  hand_drag_view_(nullptr),
  timeline_node_(nullptr),
  playhead_(0)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  ruler_ = new TimeRuler(true);
  connect(ruler_, SIGNAL(TimeChanged(const int64_t&)), this, SIGNAL(TimeChanged(const int64_t&)));
  connect(ruler_, SIGNAL(TimeChanged(const int64_t&)), this, SLOT(UpdateInternalTime(const int64_t&)));
  layout->addWidget(ruler_);

  // Create list of TimelineViews - these MUST correspond to the ViewType enum

  QSplitter* view_splitter = new QSplitter(Qt::Vertical);
  view_splitter->setChildrenCollapsible(false);
  layout->addWidget(view_splitter);

  // Video view
  views_.append(new TimelineView(kTrackTypeVideo, Qt::AlignBottom));

  // Audio view
  views_.append(new TimelineView(kTrackTypeAudio, Qt::AlignTop));

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

  // Global scrollbar
  horizontal_scroll_ = new QScrollBar(Qt::Horizontal);
  connect(horizontal_scroll_, SIGNAL(valueChanged(int)), ruler_, SLOT(SetScroll(int)));
  connect(views_.first()->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)), horizontal_scroll_, SLOT(setRange(int, int)));
  layout->addWidget(horizontal_scroll_);

  foreach (TimelineView* view, views_) {
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    view_splitter->addWidget(view);

    connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)), ruler_, SLOT(SetScroll(int)));
    connect(view, SIGNAL(ScaleChanged(double)), this, SLOT(SetScale(double)));
    connect(ruler_, SIGNAL(TimeChanged(const int64_t&)), view, SLOT(SetTime(const int64_t&)));
    connect(view, SIGNAL(TimeChanged(const int64_t&)), ruler_, SLOT(SetTime(const int64_t&)));
    connect(view, SIGNAL(TimeChanged(const int64_t&)), this, SIGNAL(TimeChanged(const int64_t&)));
    connect(horizontal_scroll_, SIGNAL(valueChanged(int)), view->horizontalScrollBar(), SLOT(setValue(int)));
    connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)), horizontal_scroll_, SLOT(setValue(int)));

    connect(view, SIGNAL(MousePressed(TimelineViewMouseEvent*)), this, SLOT(ViewMousePressed(TimelineViewMouseEvent*)));
    connect(view, SIGNAL(MouseMoved(TimelineViewMouseEvent*)), this, SLOT(ViewMouseMoved(TimelineViewMouseEvent*)));
    connect(view, SIGNAL(MouseReleased(TimelineViewMouseEvent*)), this, SLOT(ViewMouseReleased(TimelineViewMouseEvent*)));
    connect(view, SIGNAL(MouseDoubleClicked(TimelineViewMouseEvent*)), this, SLOT(ViewMouseDoubleClicked(TimelineViewMouseEvent*)));
    connect(view, SIGNAL(DragEntered(TimelineViewMouseEvent*)), this, SLOT(ViewDragEntered(TimelineViewMouseEvent*)));
    connect(view, SIGNAL(DragMoved(TimelineViewMouseEvent*)), this, SLOT(ViewDragMoved(TimelineViewMouseEvent*)));
    connect(view, SIGNAL(DragLeft(QDragLeaveEvent*)), this, SLOT(ViewDragLeft(QDragLeaveEvent*)));
    connect(view, SIGNAL(DragDropped(TimelineViewMouseEvent*)), this, SLOT(ViewDragDropped(TimelineViewMouseEvent*)));

    // Connect each view's scroll to each other
    foreach (TimelineView* other_view, views_) {
      if (view != other_view) {
        connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)), other_view->horizontalScrollBar(), SLOT(setValue(int)));
      }
    }
  }

  // Split viewer 50/50
  view_splitter->setSizes({INT_MAX, INT_MAX});

  // FIXME: Magic number
  SetScale(90.0);
}

void TimelineWidget::Clear()
{
  SetTimebase(0);

  QMapIterator<Block*, TimelineViewBlockItem*> iterator(block_items_);

  while (iterator.hasNext()) {
    iterator.next();

    if (iterator.value() != nullptr) {
      delete iterator.value();
    }
  }

  block_items_.clear();
}

void TimelineWidget::SetTimebase(const rational &timebase)
{
  SetTimebaseInternal(timebase);

  ruler_->SetTimebase(timebase);

  foreach (TimelineView* view, views_) {
    view->SetTimebase(timebase);
  }
}

void TimelineWidget::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);

  horizontal_scroll_->setPageStep(horizontal_scroll_->width());
}

void TimelineWidget::SetTime(const int64_t &timestamp)
{
  ruler_->SetTime(timestamp);

  foreach (TimelineView* view, views_) {
    view->SetTime(timestamp);
  }

  UpdateInternalTime(timestamp);
}

void TimelineWidget::ConnectTimelineNode(TimelineOutput *node)
{
  if (timeline_node_ != nullptr) {
    disconnect(timeline_node_, SIGNAL(LengthChanged(const rational&)), this, SLOT(UpdateTimelineLength(const rational&)));
    disconnect(timeline_node_, SIGNAL(BlockAdded(Block*, TrackReference)), this, SLOT(AddBlock(Block*, TrackReference)));
    disconnect(timeline_node_, SIGNAL(BlockRemoved(Block*)), this, SLOT(RemoveBlock(Block*)));
    disconnect(timeline_node_, SIGNAL(TrackAdded(TrackOutput*, TrackType)), this, SLOT(AddTrack(TrackOutput*, TrackType)));
    disconnect(timeline_node_, SIGNAL(TrackRemoved(TrackOutput*)), this, SLOT(RemoveTrack(TrackOutput*)));
    disconnect(timeline_node_, SIGNAL(TimebaseChanged(const rational&)), this, SLOT(SetTimebase(const rational&)));

    SetTimebase(0);

    Clear();
  }

  timeline_node_ = node;

  if (timeline_node_ != nullptr) {
    connect(timeline_node_, SIGNAL(LengthChanged(const rational&)), this, SLOT(UpdateTimelineLength(const rational&)));
    connect(timeline_node_, SIGNAL(BlockAdded(Block*, TrackReference)), this, SLOT(AddBlock(Block*, TrackReference)));
    connect(timeline_node_, SIGNAL(BlockRemoved(Block*)), this, SLOT(RemoveBlock(Block*)));
    connect(timeline_node_, SIGNAL(TrackAdded(TrackOutput*, TrackType)), this, SLOT(AddTrack(TrackOutput*, TrackType)));
    connect(timeline_node_, SIGNAL(TrackRemoved(TrackOutput*)), this, SLOT(RemoveTrack(TrackOutput*)));
    connect(timeline_node_, SIGNAL(TimebaseChanged(const rational&)), this, SLOT(SetTimebase(const rational&)));

    SetTimebase(timeline_node_->timebase());

    for (int i=0;i<views_.size();i++) {
      TrackType track_type = static_cast<TrackType>(i);

      TimelineView* view = views_.at(i);

      view->SetEndTime(timeline_node_->timeline_length());

      // Defer to the track to make all the block UI items necessary
      foreach (TrackOutput* track, timeline_node_->track_list(track_type)->Tracks()) {
        AddTrack(track, track_type);
      }
    }
  }
}

void TimelineWidget::DisconnectTimelineNode()
{
  ConnectTimelineNode(nullptr);
}

void TimelineWidget::ZoomIn()
{
  SetScale(scale_ * 2);
}

void TimelineWidget::ZoomOut()
{
  SetScale(scale_ * 0.5);
}

void TimelineWidget::SelectAll()
{
  foreach (TimelineView* view, views_) {
    view->SelectAll();
  }
}

void TimelineWidget::DeselectAll()
{
  foreach (TimelineView* view, views_) {
    view->DeselectAll();
  }
}

void TimelineWidget::RippleToIn()
{
  RippleEditTo(olive::timeline::kTrimIn, false);
}

void TimelineWidget::RippleToOut()
{
  RippleEditTo(olive::timeline::kTrimOut, false);
}

void TimelineWidget::EditToIn()
{
  RippleEditTo(olive::timeline::kTrimIn, true);
}

void TimelineWidget::EditToOut()
{
  RippleEditTo(olive::timeline::kTrimOut, true);
}

void TimelineWidget::GoToPrevCut()
{
  if (timeline_node_ == nullptr) {
    return;
  }

  if (playhead_ == 0) {
    return;
  }

  int64_t closest_cut = 0;

  foreach (TrackOutput* track, timeline_node_->Tracks()) {
    int64_t this_track_closest_cut = 0;

    foreach (Block* block, track->Blocks()) {
      int64_t block_out_ts = olive::time_to_timestamp(block->out(), timebase());

      if (block_out_ts < playhead_) {
        this_track_closest_cut = block_out_ts;
      } else {
        break;
      }
    }

    closest_cut = qMax(closest_cut, this_track_closest_cut);
  }

  SetTimeAndSignal(closest_cut);
}

void TimelineWidget::GoToNextCut()
{
  if (timeline_node_ == nullptr) {
    return;
  }

  int64_t closest_cut = INT64_MAX;

  foreach (TrackOutput* track, timeline_node_->Tracks()) {
    int64_t this_track_closest_cut = olive::time_to_timestamp(track->track_length(), timebase());

    if (this_track_closest_cut <= playhead_) {
      this_track_closest_cut = INT64_MAX;
    }

    foreach (Block* block, track->Blocks()) {
      int64_t block_in_ts = olive::time_to_timestamp(block->in(), timebase());

      if (block_in_ts > playhead_) {
        this_track_closest_cut = block_in_ts;
        break;
      }
    }

    closest_cut = qMin(closest_cut, this_track_closest_cut);
  }

  if (closest_cut < INT64_MAX) {
    SetTimeAndSignal(closest_cut);
  }
}

void TimelineWidget::SplitAtPlayhead()
{
  rational playhead_time = olive::timestamp_to_time(playhead_, timebase());

  QList<TimelineViewBlockItem *> selected_blocks = GetSelectedBlocks();

  // Prioritize blocks that are selected and overlap the playhead
  QVector<Block*> blocks_to_split;
  QVector<bool> block_is_selected;

  bool some_blocks_are_selected = false;

  // Get all blocks at the playhead
  foreach (TrackOutput* track, timeline_node_->Tracks()) {
    Block* b = track->BlockContainingTime(playhead_time);

    if (b) {
      bool selected = false;

      // See if this block is selected
      foreach (TimelineViewBlockItem* item, selected_blocks) {
        if (item->block() == b) {
          some_blocks_are_selected = true;
          selected = true;
          break;
        }
      }

      blocks_to_split.append(b);
      block_is_selected.append(selected);
    }
  }

  // If some blocks are selected, we prioritize those and don't split the blocks that aren't
  if (some_blocks_are_selected) {
    for (int i=0;i<block_is_selected.size();i++) {
      if (!block_is_selected.at(i)) {
        blocks_to_split.removeAt(i);
        block_is_selected.removeAt(i);
        i--;
      }
    }
  }

  if (!blocks_to_split.isEmpty()) {
    olive::undo_stack.push(new BlockSplitPreservingLinksCommand(blocks_to_split, {playhead_time}));
  }
}

QList<TimelineViewBlockItem *> TimelineWidget::GetSelectedBlocks()
{
  QList<TimelineViewBlockItem *> list;

  QMapIterator<Block*, TimelineViewBlockItem*> iterator(block_items_);

  while (iterator.hasNext()) {
    iterator.next();

    TimelineViewBlockItem* item = iterator.value();

    if (item != nullptr && item->isSelected()) {
      list.append(item);
    }
  }

  return list;
}

void TimelineWidget::RippleEditTo(olive::timeline::MovementMode mode, bool insert_gaps)
{
  rational playhead_time = olive::timestamp_to_time(playhead_, timebase());

  rational closest_point_to_playhead;
  if (mode == olive::timeline::kTrimIn) {
    closest_point_to_playhead = 0;
  } else {
    closest_point_to_playhead = RATIONAL_MAX;
  }

  foreach (TrackOutput* track, timeline_node_->Tracks()) {
    Block* b = track->NearestBlockBefore(playhead_time);

    if (b != nullptr) {
      if (mode == olive::timeline::kTrimIn) {
        closest_point_to_playhead = qMax(b->in(), closest_point_to_playhead);
      } else {
        closest_point_to_playhead = qMin(b->out(), closest_point_to_playhead);
      }
    }
  }

  QUndoCommand* command = new QUndoCommand();

  if (closest_point_to_playhead == playhead_time) {
    // Remove one frame only
    if (mode == olive::timeline::kTrimIn) {
      playhead_time += timebase();
    } else {
      playhead_time -= timebase();
    }
  }

  rational in_ripple = qMin(closest_point_to_playhead, playhead_time);
  rational out_ripple = qMax(closest_point_to_playhead, playhead_time);
  rational ripple_length = out_ripple - in_ripple;

  foreach (TrackOutput* track, timeline_node_->Tracks()) {
    TrackRippleRemoveAreaCommand* ripple_command = new TrackRippleRemoveAreaCommand(track,
                                                                                    in_ripple,
                                                                                    out_ripple,
                                                                                    command);

    if (insert_gaps) {
      GapBlock* gap = new GapBlock();
      gap->set_length(ripple_length);
      ripple_command->SetInsert(gap);
    }
  }

  olive::undo_stack.pushIfHasChildren(command);

  if (mode == olive::timeline::kTrimIn && !insert_gaps) {
    int64_t new_time = olive::time_to_timestamp(closest_point_to_playhead, timebase());

    SetTimeAndSignal(new_time);
  }
}

void TimelineWidget::SetTimeAndSignal(const int64_t &t)
{
  SetTime(t);
  emit TimeChanged(t);
}

TrackOutput *TimelineWidget::GetTrackFromReference(const TrackReference &ref)
{
  return timeline_node_->track_list(ref.type())->TrackAt(ref.index());
}

int TimelineWidget::GetTrackY(const TrackReference &ref)
{
  return views_.at(ref.type())->GetTrackY(ref.index());
}

int TimelineWidget::GetTrackHeight(const TrackReference &ref)
{
  return views_.at(ref.type())->GetTrackHeight(ref.index());
}

void TimelineWidget::CenterOn(qreal scene_pos)
{
  horizontal_scroll_->setValue(qRound(scene_pos - horizontal_scroll_->width()/2));
}

void TimelineWidget::SetScale(double scale)
{
  scale_ = scale;

  ruler_->SetScale(scale_);

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

  foreach (TimelineView* view, views_) {
    view->SetScale(scale_);
  }
}

void TimelineWidget::ClearGhosts()
{
  if (!ghost_items_.isEmpty()) {
    foreach (TimelineViewGhostItem* ghost, ghost_items_) {
      delete ghost;
    }

    ghost_items_.clear();
  }
}

bool TimelineWidget::HasGhosts()
{
  return !ghost_items_.isEmpty();
}

void TimelineWidget::UpdateInternalTime(const int64_t &timestamp)
{
  playhead_ = timestamp;
}

void TimelineWidget::UpdateTimelineLength(const rational &length)
{
  foreach (TimelineView* view, views_) {
    view->SetEndTime(length);
  }
}

TimelineWidget::Tool *TimelineWidget::GetActiveTool()
{
  return tools_.at(olive::core.tool()).get();
}

void TimelineWidget::ViewMousePressed(TimelineViewMouseEvent *event)
{
  active_tool_ = GetActiveTool();

  if (timeline_node_ != nullptr && active_tool_ != nullptr) {
    active_tool_->MousePress(event);
  }
}

void TimelineWidget::ViewMouseMoved(TimelineViewMouseEvent *event)
{
  if (timeline_node_ != nullptr && active_tool_ != nullptr) {
    active_tool_->MouseMove(event);
  }
}

void TimelineWidget::ViewMouseReleased(TimelineViewMouseEvent *event)
{
  if (timeline_node_ != nullptr && active_tool_ != nullptr) {
    active_tool_->MouseRelease(event);
  }
}

void TimelineWidget::ViewMouseDoubleClicked(TimelineViewMouseEvent *event)
{
  if (timeline_node_ != nullptr && active_tool_ != nullptr) {
    active_tool_->MouseDoubleClick(event);
  }
}

void TimelineWidget::ViewDragEntered(TimelineViewMouseEvent *event)
{
  if (timeline_node_ != nullptr) {
    import_tool_->DragEnter(event);
  }
}

void TimelineWidget::ViewDragMoved(TimelineViewMouseEvent *event)
{
  if (timeline_node_ != nullptr) {
    import_tool_->DragMove(event);
  }
}

void TimelineWidget::ViewDragLeft(QDragLeaveEvent *event)
{
  if (timeline_node_ != nullptr) {
    import_tool_->DragLeave(event);
  }
}

void TimelineWidget::ViewDragDropped(TimelineViewMouseEvent *event)
{
  if (timeline_node_ != nullptr) {
    import_tool_->DragDrop(event);
  }
}

void TimelineWidget::AddBlock(Block *block, TrackReference track)
{
  switch (block->type()) {
  case Block::kClip:
  case Block::kTransition:
  case Block::kGap:
  {
    TimelineViewBlockItem* item = new TimelineViewBlockItem();

    // Set up clip with view parameters (clip item will automatically size its rect accordingly)
    item->SetBlock(block);
    item->SetYCoords(GetTrackY(track), GetTrackHeight(track));
    item->SetScale(scale_);
    item->SetTrack(track);

    // Add to list of clip items that can be iterated through
    block_items_.insert(block, item);

    // Add item to graphics scene
    views_.at(track.type())->scene()->addItem(item);

    connect(block, SIGNAL(Refreshed()), this, SLOT(BlockChanged()));
    break;
  }
  case Block::kTrack:
    // Do nothing
    break;
  }
}

void TimelineWidget::RemoveBlock(Block *block)
{
  delete block_items_[block];

  block_items_.remove(block);
}

void TimelineWidget::AddTrack(TrackOutput *track, TrackType type)
{
  foreach (Block* b, track->Blocks()) {
    AddBlock(b, TrackReference(type, track->Index()));
  }
}

void TimelineWidget::RemoveTrack(TrackOutput *track)
{
  foreach (Block* b, track->Blocks()) {
    RemoveBlock(b);
  }
}

void TimelineWidget::BlockChanged()
{
  TimelineViewRect* rect = block_items_[static_cast<Block*>(sender())];

  if (rect != nullptr) {
    rect->UpdateRect();
  }
}

void TimelineWidget::AddGhost(TimelineViewGhostItem *ghost)
{
  ghost->SetScale(scale_);
  ghost_items_.append(ghost);
  views_.at(ghost->Track().type())->scene()->addItem(ghost);
}

void TimelineWidget::SetBlockLinksSelected(Block* block, bool selected)
{
  TimelineViewBlockItem* link_item;

  foreach (Block* link, block->linked_clips()) {
    if ((link_item = block_items_[link]) != nullptr) {
      link_item->setSelected(selected);
    }
  }
}

void TimelineWidget::StartRubberBandSelect(bool select_links)
{
  drag_origin_ = QCursor::pos();
  rubberband_.show();

  MoveRubberBandSelect(select_links);
}

void TimelineWidget::MoveRubberBandSelect(bool select_links)
{
  QPoint rubberband_now = QCursor::pos();

  rubberband_.setGeometry(QRect(mapFromGlobal(drag_origin_), mapFromGlobal(rubberband_now)).normalized());

  QList<QGraphicsItem*> new_selected_list;

  foreach (TimelineView* view, views_) {
    // Map global mouse coordinates to viewport

    QRect mapped_rect(view->viewport()->mapFromGlobal(drag_origin_),
                      view->viewport()->mapFromGlobal(rubberband_now));

    // Normalize and get items in rect
    QList<QGraphicsItem*> rubberband_items = view->items(mapped_rect.normalized());

    new_selected_list.append(rubberband_items);
  }

  foreach (QGraphicsItem* item, rubberband_now_selected_) {
    item->setSelected(false);
  }

  foreach (QGraphicsItem* item, new_selected_list) {
    item->setSelected(true);

    if (select_links) {
      // Select the block's links
      Block* b = static_cast<TimelineViewBlockItem*>(item)->block();
      SetBlockLinksSelected(b, true);

      // Add its links to the list
      TimelineViewBlockItem* link_item;
      foreach (Block* link, b->linked_clips()) {
        if ((link_item = block_items_[link]) != nullptr) {
          if (!new_selected_list.contains(link_item)) {
            new_selected_list.append(link_item);
          }
        }
      }
    }
  }

  rubberband_now_selected_ = new_selected_list;
}

void TimelineWidget::EndRubberBandSelect(bool select_links)
{
  MoveRubberBandSelect(select_links);
  rubberband_.hide();
  rubberband_now_selected_.clear();
}

void TimelineWidget::StartHandDrag()
{
  // Determine which view to hand drag by which is under the cursor now
  foreach (TimelineView* view, views_) {
    if (view->underMouse()) {
      hand_drag_view_ = view;
      hand_drag_view_origin_ = view->GetScrollCoordinates();
      drag_origin_ = QCursor::pos();
      break;
    }
  }
}

void TimelineWidget::MoveHandDrag()
{
  if (hand_drag_view_ == nullptr) {
    return;
  }

  // Drag the view if we found one in StartHandDrag()
  hand_drag_view_->SetScrollCoordinates(hand_drag_view_origin_ + (drag_origin_ - QCursor::pos()));
}

void TimelineWidget::EndHandDrag()
{
  hand_drag_view_ = nullptr;
}
