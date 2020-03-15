#include "timelinewidget.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QtMath>

#include "core.h"
#include "common/timecodefunctions.h"
#include "dialog/speedduration/speedduration.h"
#include "node/block/transition/transition.h"
#include "tool/tool.h"
#include "trackview/trackview.h"
#include "widget/menu/menu.h"
#include "widget/nodeview/nodeviewundo.h"

TimelineWidget::TimelineWidget(QWidget *parent) :
  TimeBasedWidget(true, true, parent),
  rubberband_(QRubberBand::Rectangle, this),
  active_tool_(nullptr)
{
  QVBoxLayout* vert_layout = new QVBoxLayout(this);
  vert_layout->setSpacing(0);
  vert_layout->setMargin(0);

  QHBoxLayout* ruler_and_time_layout = new QHBoxLayout();
  vert_layout->addLayout(ruler_and_time_layout);

  timecode_label_ = new TimeSlider();
  timecode_label_->SetAlignment(Qt::AlignCenter);
  timecode_label_->setVisible(false);
  connect(timecode_label_, &TimeSlider::ValueChanged, this, &TimelineWidget::SetTimeAndSignal);
  ruler_and_time_layout->addWidget(timecode_label_);

  ruler_and_time_layout->addWidget(ruler());

  // Create list of TimelineViews - these MUST correspond to the ViewType enum

  QSplitter* view_splitter = new QSplitter(Qt::Vertical);
  view_splitter->setChildrenCollapsible(false);
  vert_layout->addWidget(view_splitter);

  // Video view
  views_.append(new TimelineAndTrackView(Qt::AlignBottom));

  // Audio view
  views_.append(new TimelineAndTrackView(Qt::AlignTop));

  // Create tools
  tools_.resize(::Tool::kCount);
  tools_.fill(nullptr);

  tools_.replace(::Tool::kPointer, new PointerTool(this));
  tools_.replace(::Tool::kEdit, new EditTool(this));
  tools_.replace(::Tool::kRipple, new RippleTool(this));
  tools_.replace(::Tool::kRolling, new RollingTool(this));
  tools_.replace(::Tool::kRazor, new RazorTool(this));
  tools_.replace(::Tool::kSlip, new SlipTool(this));
  tools_.replace(::Tool::kSlide, new SlideTool(this));
  tools_.replace(::Tool::kZoom, new ZoomTool(this));
  tools_.replace(::Tool::kTransition, new TransitionTool(this));
  //tools_.replace(::Tool::kRecord, new PointerTool(this));  FIXME: Implement
  tools_.replace(::Tool::kAdd, new AddTool(this));

  import_tool_ = new ImportTool(this);

  // We add this to the list to make deleting all tools easier, it should never get accessed through the list under
  // normal circumstances (but *technically* its index would be Tool::kCount)
  tools_.append(import_tool_);

  // Global scrollbar
  connect(scrollbar(), &QScrollBar::valueChanged, ruler(), &TimeRuler::SetScroll);
  connect(views_.first()->view()->horizontalScrollBar(), &QScrollBar::rangeChanged, scrollbar(), &QScrollBar::setRange);
  vert_layout->addWidget(scrollbar());

  foreach (TimelineAndTrackView* tview, views_) {
    TimelineView* view = tview->view();

    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    view_splitter->addWidget(tview);

    connect(view->horizontalScrollBar(), &QScrollBar::valueChanged, ruler(), &TimeRuler::SetScroll);
    connect(view, &TimelineView::ScaleChanged, this, &TimelineWidget::SetScale);
    connect(ruler(), &TimeRuler::TimeChanged, view, &TimelineView::SetTime);
    connect(view, &TimelineView::TimeChanged, ruler(), &TimeRuler::SetTime);
    connect(view, &TimelineView::TimeChanged, this, &TimelineWidget::TimeChanged);
    connect(view, &TimelineView::customContextMenuRequested, this, &TimelineWidget::ShowContextMenu);
    connect(scrollbar(), &QScrollBar::valueChanged, view->horizontalScrollBar(), &QScrollBar::setValue);
    connect(view->horizontalScrollBar(), &QScrollBar::valueChanged, scrollbar(), &QScrollBar::setValue);
    connect(view, &TimelineView::RequestCenterScrollOnPlayhead, this, &TimelineWidget::CenterScrollOnPlayhead);

    connect(view, &TimelineView::MousePressed, this, &TimelineWidget::ViewMousePressed);
    connect(view, &TimelineView::MouseMoved, this, &TimelineWidget::ViewMouseMoved);
    connect(view, &TimelineView::MouseReleased, this, &TimelineWidget::ViewMouseReleased);
    connect(view, &TimelineView::MouseDoubleClicked, this, &TimelineWidget::ViewMouseDoubleClicked);
    connect(view, &TimelineView::DragEntered, this, &TimelineWidget::ViewDragEntered);
    connect(view, &TimelineView::DragMoved, this, &TimelineWidget::ViewDragMoved);
    connect(view, &TimelineView::DragLeft, this, &TimelineWidget::ViewDragLeft);
    connect(view, &TimelineView::DragDropped, this, &TimelineWidget::ViewDragDropped);
    connect(view, &TimelineView::SelectionChanged, this, &TimelineWidget::ViewSelectionChanged);

    connect(tview->splitter(), &QSplitter::splitterMoved, this, &TimelineWidget::UpdateHorizontalSplitters);

    // Connect each view's scroll to each other
    foreach (TimelineAndTrackView* other_tview, views_) {
      TimelineView* other_view = other_tview->view();

      if (view != other_view) {
        connect(view->horizontalScrollBar(), &QScrollBar::valueChanged, other_view->horizontalScrollBar(), &QScrollBar::setValue);
      }
    }
  }

  // Split viewer 50/50
  view_splitter->setSizes({INT_MAX, INT_MAX});

  // FIXME: Magic number
  SetMaximumScale(TimelineViewBase::kMaximumScale);
  SetScale(90.0);
}

TimelineWidget::~TimelineWidget()
{
  // Ensure no blocks are selected before any child widgets are destroyed (prevents corrupt ViewSelectionChanged() signal)
  Clear();

  qDeleteAll(tools_);
}

void TimelineWidget::Clear()
{
  SetTimebase(0);

  QMap<Block*, TimelineViewBlockItem*>::iterator iterator = block_items_.begin();

  while (iterator != block_items_.end()) {
    TimelineViewBlockItem* item = iterator.value();

    iterator = block_items_.erase(iterator);

    delete item;
  }

  block_items_.clear();
}

void TimelineWidget::TimebaseChangedEvent(const rational &timebase)
{
  TimeBasedWidget::TimebaseChangedEvent(timebase);

  timecode_label_->SetTimebase(timebase);

  timecode_label_->setVisible(!timebase.isNull());

  foreach (TimelineAndTrackView* view, views_) {
    view->view()->SetTimebase(timebase);
  }
}

void TimelineWidget::resizeEvent(QResizeEvent *event)
{
  TimeBasedWidget::resizeEvent(event);

  // Update timecode label size
  UpdateTimecodeWidthFromSplitters(views_.first()->splitter());
}

void TimelineWidget::TimeChangedEvent(const int64_t& timestamp)
{
  foreach (TimelineAndTrackView* view, views_) {
    view->view()->SetTime(timestamp);
  }

  timecode_label_->SetValue(timestamp);
}

void TimelineWidget::ScaleChangedEvent(const double &scale)
{
  TimeBasedWidget::ScaleChangedEvent(scale);

  QMapIterator<Block*, TimelineViewBlockItem*> iterator(block_items_);

  while (iterator.hasNext()) {
    iterator.next();

    if (iterator.value() != nullptr) {
      iterator.value()->SetScale(scale);
    }
  }

  foreach (TimelineViewGhostItem* ghost, ghost_items_) {
    ghost->SetScale(scale);
  }

  foreach (TimelineAndTrackView* view, views_) {
    view->view()->SetScale(scale);
  }
}

void TimelineWidget::ConnectNodeInternal(ViewerOutput *n)
{
  connect(n, &ViewerOutput::LengthChanged, this, &TimelineWidget::UpdateTimelineLength);
  connect(n, &ViewerOutput::BlockAdded, this, &TimelineWidget::AddBlock);
  connect(n, &ViewerOutput::BlockRemoved, this, &TimelineWidget::RemoveBlock);
  connect(n, &ViewerOutput::TrackAdded, this, &TimelineWidget::AddTrack);
  connect(n, &ViewerOutput::TrackRemoved, this, &TimelineWidget::RemoveTrack);
  connect(n, &ViewerOutput::TimebaseChanged, this, &TimelineWidget::SetTimebase);
  connect(n, &ViewerOutput::TrackHeightChanged, this, &TimelineWidget::TrackHeightChanged);

  SetTimebase(n->video_params().time_base());
  UpdateTimelineLength(n->Length());

  for (int i=0;i<views_.size();i++) {
    Timeline::TrackType track_type = static_cast<Timeline::TrackType>(i);
    TimelineView* view = views_.at(i)->view();
    TrackList* track_list = n->track_list(track_type);
    TrackView* track_view = views_.at(i)->track_view();

    track_view->ConnectTrackList(track_list);
    view->ConnectTrackList(track_list);
    view->SetEndTime(n->Length());

    // Defer to the track to make all the block UI items necessary
    foreach (TrackOutput* track, n->track_list(track_type)->Tracks()) {
      AddTrack(track, track_type);
    }
  }
}

void TimelineWidget::DisconnectNodeInternal(ViewerOutput *n)
{
  disconnect(n, &ViewerOutput::LengthChanged, this, &TimelineWidget::UpdateTimelineLength);
  disconnect(n, &ViewerOutput::BlockAdded, this, &TimelineWidget::AddBlock);
  disconnect(n, &ViewerOutput::BlockRemoved, this, &TimelineWidget::RemoveBlock);
  disconnect(n, &ViewerOutput::TrackAdded, this, &TimelineWidget::AddTrack);
  disconnect(n, &ViewerOutput::TrackRemoved, this, &TimelineWidget::RemoveTrack);
  disconnect(n, &ViewerOutput::TimebaseChanged, this, &TimelineWidget::SetTimebase);
  disconnect(n, &ViewerOutput::TrackHeightChanged, this, &TimelineWidget::TrackHeightChanged);

  SetTimebase(0);

  Clear();

  for (int i=0;i<views_.size();i++) {
    TrackView* track_view = views_.at(i)->track_view();
    track_view->DisconnectTrackList();
  }
}

void TimelineWidget::SelectAll()
{
  foreach (TimelineAndTrackView* view, views_) {
    view->view()->SelectAll();
  }
}

void TimelineWidget::DeselectAll()
{
  foreach (TimelineAndTrackView* view, views_) {
    view->view()->DeselectAll();
  }
}

void TimelineWidget::RippleToIn()
{
  RippleEditTo(Timeline::kTrimIn, false);
}

void TimelineWidget::RippleToOut()
{
  RippleEditTo(Timeline::kTrimOut, false);
}

void TimelineWidget::EditToIn()
{
  RippleEditTo(Timeline::kTrimIn, true);
}

void TimelineWidget::EditToOut()
{
  RippleEditTo(Timeline::kTrimOut, true);
}

void TimelineWidget::SplitAtPlayhead()
{
  if (!GetConnectedNode()) {
    return;
  }

  rational playhead_time = Timecode::timestamp_to_time(GetTimestamp(), timebase());

  QList<TimelineViewBlockItem *> selected_blocks = GetSelectedBlocks();

  // Prioritize blocks that are selected and overlap the playhead
  QVector<Block*> blocks_to_split;
  QVector<bool> block_is_selected;

  bool some_blocks_are_selected = false;

  // Get all blocks at the playhead
  foreach (TrackOutput* track, GetConnectedNode()->Tracks()) {
    Block* b = track->BlockContainingTime(playhead_time);

    if (b && b->type() == Block::kClip) {
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
    Core::instance()->undo_stack()->push(new BlockSplitPreservingLinksCommand(blocks_to_split, {playhead_time}));
  }
}

void TimelineWidget::DeleteSelectedInternal(const QList<Block *> &blocks,
                                            bool transition_aware,
                                            bool remove_from_graph,
                                            QUndoCommand *command)
{
  foreach (Block* b, blocks) {
    TrackOutput* original_track = TrackOutput::TrackFromBlock(b);

    if (transition_aware && b->type() == Block::kTransition) {
      // Deleting transitions restores their in/out offsets to their attached blocks
      TransitionBlock* transition = static_cast<TransitionBlock*>(b);

      // Ripple remove transition
      new TrackRippleRemoveBlockCommand(original_track,
                                        transition,
                                        command);

      // Resize attached blocks to make up length
      if (transition->connected_in_block()) {
        new BlockResizeWithMediaInCommand(transition->connected_in_block(),
                                          transition->connected_in_block()->length() + transition->in_offset(),
                                          command);
      }

      if (transition->connected_out_block()) {
        new BlockResizeCommand(transition->connected_out_block(),
                               transition->connected_out_block()->length() + transition->out_offset(),
                               command);
      }
    } else {
      // Make new gap and replace old Block with it for now
      GapBlock* gap = new GapBlock();
      gap->set_length_and_media_out(b->length());

      new NodeAddCommand(static_cast<NodeGraph*>(b->parent()),
                         gap,
                         command);

      new TrackReplaceBlockCommand(original_track,
                                   b,
                                   gap,
                                   command);
    }

    if (remove_from_graph) {
      new BlockUnlinkAllCommand(b, command);

      new NodeRemoveWithExclusiveDeps(static_cast<NodeGraph*>(b->parent()), b, command);
    }
  }
}

void TimelineWidget::DeleteSelected(bool ripple)
{
  QList<TimelineViewBlockItem *> selected_list = GetSelectedBlocks();
  QList<Block*> blocks_to_delete;
  QList<TrackReference> tracks_affected;

  foreach (TimelineViewBlockItem* item, selected_list) {
    Block* b = item->block();

    blocks_to_delete.append(b);

    if (!tracks_affected.contains(item->Track())) {
      tracks_affected.append(item->Track());
    }
  }

  // No-op if nothing is selected
  if (blocks_to_delete.isEmpty()) {
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  // Replace blocks with gaps (effectively deleting them)
  DeleteSelectedInternal(blocks_to_delete, true, true, command);

  // Clean each track
  foreach (const TrackReference& track, tracks_affected) {
    new TrackCleanGapsCommand(GetConnectedNode()->track_list(track.type()),
                              track.index(),
                              command);
  }

  // Insert ripple command now that it's all cleaned up gaps
  if (ripple) {
    TimeRangeList range_list;

    foreach (Block* b, blocks_to_delete) {
      range_list.InsertTimeRange(TimeRange(b->in(), b->out()));
    }

    new TimelineRippleDeleteGapsAtRegionsCommand(GetConnectedNode(), range_list, command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void TimelineWidget::IncreaseTrackHeight()
{
  if (!GetConnectedNode()) {
    return;
  }

  QVector<TrackOutput*> all_tracks = GetConnectedNode()->Tracks();

  // Increase the height of each track by one "unit"
  foreach (TrackOutput* t, all_tracks) {
    t->SetTrackHeight(t->GetTrackHeight() + t->GetTrackHeightIncrement());
  }
}

void TimelineWidget::DecreaseTrackHeight()
{
  if (!GetConnectedNode()) {
    return;
  }

  QVector<TrackOutput*> all_tracks = GetConnectedNode()->Tracks();

  // Decrease the height of each track by one "unit"
  foreach (TrackOutput* t, all_tracks) {
    t->SetTrackHeight(qMax(t->GetTrackHeight() - t->GetTrackHeightIncrement(), t->GetTrackHeightMinimum()));
  }
}

void TimelineWidget::InsertFootageAtPlayhead(const QList<Footage*>& footage)
{
  import_tool_->PlaceAt(footage, GetTime(), true);
}

void TimelineWidget::OverwriteFootageAtPlayhead(const QList<Footage *> &footage)
{
  import_tool_->PlaceAt(footage, GetTime(), false);
}

void TimelineWidget::ToggleLinksOnSelected()
{
  QList<TimelineViewBlockItem*> sel = GetSelectedBlocks();

  // Prioritize unlinking

  QList<Block*> blocks;
  bool link = true;

  foreach (TimelineViewBlockItem* item, sel) {
    if (link && item->block()->HasLinks()) {
      link = false;
    }

    blocks.append(item->block());
  }

  if (link) {
    Core::instance()->undo_stack()->push(new BlockLinkManyCommand(blocks, true));
  } else {
    Core::instance()->undo_stack()->push(new BlockLinkManyCommand(blocks, false));
  }
}

QList<TimelineViewBlockItem *> TimelineWidget::GetSelectedBlocks()
{
  QList<TimelineViewBlockItem *> list;

  QMapIterator<Block*, TimelineViewBlockItem*> iterator(block_items_);

  while (iterator.hasNext()) {
    iterator.next();

    TimelineViewBlockItem* item = iterator.value();

    if (item && item->isSelected()) {
      list.append(item);
    }
  }

  return list;
}

void TimelineWidget::RippleEditTo(Timeline::MovementMode mode, bool insert_gaps)
{
  rational playhead_time = GetTime();

  rational closest_point_to_playhead;
  if (mode == Timeline::kTrimIn) {
    closest_point_to_playhead = 0;
  } else {
    closest_point_to_playhead = RATIONAL_MAX;
  }

  foreach (TrackOutput* track, GetConnectedNode()->Tracks()) {
    Block* b = track->NearestBlockBefore(playhead_time);

    if (b != nullptr) {
      if (mode == Timeline::kTrimIn) {
        closest_point_to_playhead = qMax(b->in(), closest_point_to_playhead);
      } else {
        closest_point_to_playhead = qMin(b->out(), closest_point_to_playhead);
      }
    }
  }

  QUndoCommand* command = new QUndoCommand();

  if (closest_point_to_playhead == playhead_time) {
    // Remove one frame only
    if (mode == Timeline::kTrimIn) {
      playhead_time += timebase();
    } else {
      playhead_time -= timebase();
    }
  }

  rational in_ripple = qMin(closest_point_to_playhead, playhead_time);
  rational out_ripple = qMax(closest_point_to_playhead, playhead_time);
  rational ripple_length = out_ripple - in_ripple;

  foreach (TrackOutput* track, GetConnectedNode()->Tracks()) {
    GapBlock* gap = nullptr;
    if (insert_gaps) {
      gap = new GapBlock();
      gap->set_length_and_media_out(ripple_length);
      new NodeAddCommand(static_cast<NodeGraph*>(track->parent()), gap, command);
    }

    TrackRippleRemoveAreaCommand* ripple_command = new TrackRippleRemoveAreaCommand(track,
                                                                                    in_ripple,
                                                                                    out_ripple,
                                                                                    command);

    if (insert_gaps) {
      ripple_command->SetInsert(gap);
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  if (mode == Timeline::kTrimIn && !insert_gaps) {
    int64_t new_time = Timecode::time_to_timestamp(closest_point_to_playhead, timebase());

    SetTimeAndSignal(new_time);
  }
}

TrackOutput *TimelineWidget::GetTrackFromReference(const TrackReference &ref)
{
  return GetConnectedNode()->track_list(ref.type())->TrackAt(ref.index());
}

int TimelineWidget::GetTrackY(const TrackReference &ref)
{
  return views_.at(ref.type())->view()->GetTrackY(ref.index());
}

int TimelineWidget::GetTrackHeight(const TrackReference &ref)
{
  return views_.at(ref.type())->view()->GetTrackHeight(ref.index());
}

void TimelineWidget::CenterOn(qreal scene_pos)
{
  scrollbar()->setValue(qRound(scene_pos - scrollbar()->width()/2));
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

void TimelineWidget::UpdateTimelineLength(const rational &length)
{
  foreach (TimelineAndTrackView* view, views_) {
    view->view()->SetEndTime(length);
  }
  ruler()->SetCacheStatusLength(length);
}

TimelineWidget::Tool *TimelineWidget::GetActiveTool()
{
  return tools_.at(Core::instance()->tool());
}

void TimelineWidget::ViewMousePressed(TimelineViewMouseEvent *event)
{
  active_tool_ = GetActiveTool();

  if (GetConnectedNode() && active_tool_ != nullptr) {
    active_tool_->MousePress(event);
  }
}

void TimelineWidget::ViewMouseMoved(TimelineViewMouseEvent *event)
{
  if (GetConnectedNode()) {
    if (active_tool_) {
      active_tool_->MouseMove(event);
    } else {
      // Mouse is not down, attempt a hover event
      Tool* hover_tool = GetActiveTool();

      if (hover_tool) {
        hover_tool->HoverMove(event);
      }
    }
  }
}

void TimelineWidget::ViewMouseReleased(TimelineViewMouseEvent *event)
{
  if (GetConnectedNode() && active_tool_ != nullptr) {
    active_tool_->MouseRelease(event);
    active_tool_ = nullptr;
  }
}

void TimelineWidget::ViewMouseDoubleClicked(TimelineViewMouseEvent *event)
{
  if (GetConnectedNode() && active_tool_ != nullptr) {
    active_tool_->MouseDoubleClick(event);
    active_tool_ = nullptr;
  }
}

void TimelineWidget::ViewDragEntered(TimelineViewMouseEvent *event)
{
  import_tool_->DragEnter(event);
}

void TimelineWidget::ViewDragMoved(TimelineViewMouseEvent *event)
{
  import_tool_->DragMove(event);
}

void TimelineWidget::ViewDragLeft(QDragLeaveEvent *event)
{
  import_tool_->DragLeave(event);
}

void TimelineWidget::ViewDragDropped(TimelineViewMouseEvent *event)
{
  import_tool_->DragDrop(event);
}

void TimelineWidget::AddBlock(Block *block, TrackReference track)
{
  switch (block->type()) {
  case Block::kClip:
  case Block::kTransition:
  case Block::kGap:
  {
    // Set up clip with view parameters (clip item will automatically size its rect accordingly)
    TimelineViewBlockItem* item = new TimelineViewBlockItem(block);

    item->SetYCoords(GetTrackY(track), GetTrackHeight(track));
    item->SetScale(GetScale());
    item->SetTrack(track);

    // Add to list of clip items that can be iterated through
    block_items_.insert(block, item);

    // Add item to graphics scene
    views_.at(track.type())->view()->scene()->addItem(item);

    connect(block, &Block::Refreshed, this, &TimelineWidget::BlockChanged);
    connect(block, &Block::LinksChanged, this, &TimelineWidget::PreviewUpdated);

    if (block->type() == Block::kClip) {
      connect(static_cast<ClipBlock*>(block), &ClipBlock::PreviewUpdated, this, &TimelineWidget::PreviewUpdated);
    }
    break;
  }
  }
}

void TimelineWidget::RemoveBlock(Block *block)
{
  delete block_items_[block];

  block_items_.remove(block);
}

void TimelineWidget::AddTrack(TrackOutput *track, Timeline::TrackType type)
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

void TimelineWidget::ViewSelectionChanged()
{
  if (rubberband_.isVisible()) {
    return;
  }

  QList<TimelineViewBlockItem*> selected_items = GetSelectedBlocks();
  QList<Node*> selected_blocks;

  foreach (TimelineViewBlockItem* item, selected_items) {
    selected_blocks.append(item->block());
  }

  emit SelectionChanged(selected_blocks);
}

void TimelineWidget::BlockChanged()
{
  TimelineViewRect* rect = block_items_.value(static_cast<Block*>(sender()));

  if (rect) {
    rect->UpdateRect();
  }
}

void TimelineWidget::PreviewUpdated()
{
  TimelineViewRect* rect = block_items_.value(static_cast<Block*>(sender()));

  if (rect) {
    rect->update();
  }
}

void TimelineWidget::UpdateHorizontalSplitters()
{
  QSplitter* sender_splitter = static_cast<QSplitter*>(sender());

  foreach (TimelineAndTrackView* tview, views_) {
    QSplitter* recv_splitter = tview->splitter();

    if (recv_splitter != sender_splitter) {
      recv_splitter->blockSignals(true);
      recv_splitter->setSizes(sender_splitter->sizes());
      recv_splitter->blockSignals(false);
    }
  }

  UpdateTimecodeWidthFromSplitters(sender_splitter);
}

void TimelineWidget::UpdateTimecodeWidthFromSplitters(QSplitter* s)
{
  timecode_label_->setFixedWidth(s->sizes().first() + s->handleWidth());
}

void TimelineWidget::TrackHeightChanged(Timeline::TrackType type, int index, int height)
{
  Q_UNUSED(index)
  Q_UNUSED(height)

  QMap<Block*, TimelineViewBlockItem*>::const_iterator iterator;
  TimelineView* view = views_.at(type)->view();

  for (iterator=block_items_.begin();iterator!=block_items_.end();iterator++) {
    TimelineViewBlockItem* block_item = iterator.value();

    if (block_item->Track().type() == type) {
      block_item->SetYCoords(view->GetTrackY(block_item->Track().index()),
                             view->GetTrackHeight(block_item->Track().index()));
    }
  }
}

void TimelineWidget::ShowContextMenu()
{
  Menu menu(this);

  QList<TimelineViewBlockItem*> selected = GetSelectedBlocks();

  if (!selected.isEmpty()) {
    QAction* speed_duration_action = menu.addAction(tr("Speed/Duration"));
    connect(speed_duration_action, &QAction::triggered, this, &TimelineWidget::ShowSpeedDurationDialog);
  }

  menu.exec(QCursor::pos());
}

void TimelineWidget::ShowSpeedDurationDialog()
{
  QList<TimelineViewBlockItem*> selected = GetSelectedBlocks();
  QList<ClipBlock*> selected_clips;

  foreach (TimelineViewBlockItem* item, selected) {
    if (item->block()->type() == Block::kClip) {
      selected_clips.append(static_cast<ClipBlock*>(item->block()));
    }
  }

  if (selected_clips.isEmpty()) {
    // SpeedDurationDialog expects at least one clip
    return;
  }

  SpeedDurationDialog speed_diag(timebase(), selected_clips, this);
  speed_diag.exec();
}

void TimelineWidget::DeferredScrollAction()
{
  scrollbar()->setValue(deferred_scroll_value_);
}

void TimelineWidget::AddGhost(TimelineViewGhostItem *ghost)
{
  ghost->SetScale(GetScale());
  ghost_items_.append(ghost);
  views_.at(ghost->Track().type())->view()->scene()->addItem(ghost);
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

void TimelineWidget::StartRubberBandSelect(bool enable_selecting, bool select_links)
{
  drag_origin_ = QCursor::pos();
  rubberband_.show();

  MoveRubberBandSelect(enable_selecting, select_links);
}

void TimelineWidget::MoveRubberBandSelect(bool enable_selecting, bool select_links)
{
  QPoint rubberband_now = QCursor::pos();

  rubberband_.setGeometry(QRect(mapFromGlobal(drag_origin_), mapFromGlobal(rubberband_now)).normalized());

  if (!enable_selecting) {
    return;
  }

  QList<QGraphicsItem*> new_selected_list;

  foreach (TimelineAndTrackView* tview, views_) {
    // Map global mouse coordinates to viewport
    TimelineView* view = tview->view();

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
    TimelineViewBlockItem* block_item = dynamic_cast<TimelineViewBlockItem*>(item);
    if (!block_item || block_item->block()->type() == Block::kGap) {
      continue;
    }

    TrackOutput* t = GetTrackFromReference(block_item->Track());
    if (t && t->IsLocked()) {
      continue;
    }

    block_item->setSelected(true);

    if (select_links) {
      // Select the block's links
      Block* b = block_item->block();
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

void TimelineWidget::EndRubberBandSelect(bool enable_selecting, bool select_links)
{
  MoveRubberBandSelect(enable_selecting, select_links);
  rubberband_.hide();
  rubberband_now_selected_.clear();

  ViewSelectionChanged();
}
