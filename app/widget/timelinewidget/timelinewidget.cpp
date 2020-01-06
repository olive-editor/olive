#include "timelinewidget.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QtMath>

#include "core.h"
#include "common/timecodefunctions.h"
#include "dialog/speedduration/speedduration.h"
#include "tool/tool.h"
#include "trackview/trackview.h"
#include "widget/menu/menu.h"
#include "widget/nodeview/nodeviewundo.h"

TimelineWidget::TimelineWidget(QWidget *parent) :
  QWidget(parent),
  rubberband_(QRubberBand::Rectangle, this),
  active_tool_(nullptr),
  timeline_node_(nullptr),
  playhead_(0)
{
  QVBoxLayout* vert_layout = new QVBoxLayout(this);
  vert_layout->setSpacing(0);
  vert_layout->setMargin(0);

  QHBoxLayout* ruler_and_time_layout = new QHBoxLayout();
  vert_layout->addLayout(ruler_and_time_layout);

  timecode_label_ = new TimeSlider();
  timecode_label_->SetAlignment(Qt::AlignCenter);
  timecode_label_->setVisible(false);
  connect(timecode_label_, SIGNAL(ValueChanged(int64_t)), this, SIGNAL(TimeChanged(const int64_t&)));
  connect(timecode_label_, SIGNAL(ValueChanged(int64_t)), this, SLOT(UpdateInternalTime(const int64_t&)));
  ruler_and_time_layout->addWidget(timecode_label_);

  ruler_ = new TimeRuler(true, true);
  connect(ruler_, SIGNAL(TimeChanged(const int64_t&)), this, SIGNAL(TimeChanged(const int64_t&)));
  connect(ruler_, SIGNAL(TimeChanged(const int64_t&)), this, SLOT(UpdateInternalTime(const int64_t&)));
  ruler_and_time_layout->addWidget(ruler_);

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
  horizontal_scroll_ = new QScrollBar(Qt::Horizontal);
  connect(horizontal_scroll_, SIGNAL(valueChanged(int)), ruler_, SLOT(SetScroll(int)));
  connect(views_.first()->view()->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)), horizontal_scroll_, SLOT(setRange(int, int)));
  vert_layout->addWidget(horizontal_scroll_);

  foreach (TimelineAndTrackView* tview, views_) {
    TimelineView* view = tview->view();

    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    view_splitter->addWidget(tview);

    connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)), ruler_, SLOT(SetScroll(int)));
    connect(view, SIGNAL(ScaleChanged(double)), this, SLOT(SetScale(double)));
    connect(ruler_, SIGNAL(TimeChanged(const int64_t&)), view, SLOT(SetTime(const int64_t&)));
    connect(view, SIGNAL(TimeChanged(const int64_t&)), ruler_, SLOT(SetTime(const int64_t&)));
    connect(view, SIGNAL(TimeChanged(const int64_t&)), this, SIGNAL(TimeChanged(const int64_t&)));
    connect(view, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu()));
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

    connect(tview->splitter(), SIGNAL(splitterMoved(int, int)), this, SLOT(UpdateHorizontalSplitters()));

    // Connect each view's scroll to each other
    foreach (TimelineAndTrackView* other_tview, views_) {
      TimelineView* other_view = other_tview->view();

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

TimelineWidget::~TimelineWidget()
{
  qDeleteAll(tools_);
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
  timecode_label_->SetTimebase(timebase);

  timecode_label_->setVisible(!timebase.isNull());

  foreach (TimelineAndTrackView* view, views_) {
    view->view()->SetTimebase(timebase);
  }
}

void TimelineWidget::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);

  // Update horizontal scrollbar's page step to the width of the panel
  horizontal_scroll_->setPageStep(horizontal_scroll_->width());

  // Update timecode label size
  UpdateTimecodeWidthFromSplitters(views_.first()->splitter());
}

void TimelineWidget::SetTime(int64_t timestamp)
{
  ruler_->SetTime(timestamp);

  foreach (TimelineAndTrackView* view, views_) {
    view->view()->SetTime(timestamp);
  }

  UpdateInternalTime(timestamp);
}

void TimelineWidget::ConnectTimelineNode(ViewerOutput *node)
{
  if (timeline_node_ != nullptr) {
    disconnect(timeline_node_, SIGNAL(LengthChanged(const rational&)), this, SLOT(UpdateTimelineLength(const rational&)));
    disconnect(timeline_node_, SIGNAL(BlockAdded(Block*, TrackReference)), this, SLOT(AddBlock(Block*, TrackReference)));
    disconnect(timeline_node_, SIGNAL(BlockRemoved(Block*)), this, SLOT(RemoveBlock(Block*)));
    disconnect(timeline_node_, SIGNAL(TrackAdded(TrackOutput*, TrackType)), this, SLOT(AddTrack(TrackOutput*, TrackType)));
    disconnect(timeline_node_, SIGNAL(TrackRemoved(TrackOutput*)), this, SLOT(RemoveTrack(TrackOutput*)));
    disconnect(timeline_node_, SIGNAL(TimebaseChanged(const rational&)), this, SLOT(SetTimebase(const rational&)));
    disconnect(timeline_node_, SIGNAL(TrackHeightChanged(TrackType, int, int)), this, SLOT(TrackHeightChanged(TrackType, int, int)));

    SetTimebase(0);

    Clear();

    for (int i=0;i<views_.size();i++) {
      TrackView* track_view = views_.at(i)->track_view();
      track_view->DisconnectTrackList();
    }
  }

  timeline_node_ = node;

  if (timeline_node_ != nullptr) {
    connect(timeline_node_, SIGNAL(LengthChanged(const rational&)), this, SLOT(UpdateTimelineLength(const rational&)));
    connect(timeline_node_, SIGNAL(BlockAdded(Block*, TrackReference)), this, SLOT(AddBlock(Block*, TrackReference)));
    connect(timeline_node_, SIGNAL(BlockRemoved(Block*)), this, SLOT(RemoveBlock(Block*)));
    connect(timeline_node_, SIGNAL(TrackAdded(TrackOutput*, TrackType)), this, SLOT(AddTrack(TrackOutput*, TrackType)));
    connect(timeline_node_, SIGNAL(TrackRemoved(TrackOutput*)), this, SLOT(RemoveTrack(TrackOutput*)));
    connect(timeline_node_, SIGNAL(TimebaseChanged(const rational&)), this, SLOT(SetTimebase(const rational&)));
    connect(timeline_node_, SIGNAL(TrackHeightChanged(TrackType, int, int)), this, SLOT(TrackHeightChanged(TrackType, int, int)));

    SetTimebase(timeline_node_->video_params().time_base());

    for (int i=0;i<views_.size();i++) {
      Timeline::TrackType track_type = static_cast<Timeline::TrackType>(i);
      TimelineView* view = views_.at(i)->view();
      TrackList* track_list = timeline_node_->track_list(track_type);
      TrackView* track_view = views_.at(i)->track_view();

      track_view->ConnectTrackList(track_list);
      view->ConnectTrackList(track_list);
      view->SetEndTime(timeline_node_->Length());

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
      int64_t block_out_ts = Timecode::time_to_timestamp(block->out(), timebase());

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
    int64_t this_track_closest_cut = Timecode::time_to_timestamp(track->track_length(), timebase());

    if (this_track_closest_cut <= playhead_) {
      this_track_closest_cut = INT64_MAX;
    }

    foreach (Block* block, track->Blocks()) {
      int64_t block_in_ts = Timecode::time_to_timestamp(block->in(), timebase());

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
  rational playhead_time = Timecode::timestamp_to_time(playhead_, timebase());

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
    Core::instance()->undo_stack()->push(new BlockSplitPreservingLinksCommand(blocks_to_split, {playhead_time}));
  }
}

void TimelineWidget::DeleteSelectedInternal(const QList<Block *> blocks, bool remove_from_graph, QUndoCommand *command)
{
  foreach (Block* b, blocks) {
    TrackOutput* original_track = TrackOutput::TrackFromBlock(b);

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

    if (remove_from_graph) {
      new NodeRemoveWithExclusiveDeps(static_cast<NodeGraph*>(b->parent()), b, command);
    }
  }
}

void TimelineWidget::DeleteSelected()
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
  DeleteSelectedInternal(blocks_to_delete, true, command);

  // Clean each track
  foreach (const TrackReference& track, tracks_affected) {
    new TrackCleanGapsCommand(timeline_node_->track_list(track.type()),
                              track.index(),
                              command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void TimelineWidget::IncreaseTrackHeight()
{
  if (!timeline_node_) {
    return;
  }

  QVector<TrackOutput*> all_tracks = timeline_node_->Tracks();

  // Increase the height of each track by one "unit"
  foreach (TrackOutput* t, all_tracks) {
    t->SetTrackHeight(t->GetTrackHeight() + t->GetTrackHeightIncrement());
  }
}

void TimelineWidget::DecreaseTrackHeight()
{
  if (!timeline_node_) {
    return;
  }

  QVector<TrackOutput*> all_tracks = timeline_node_->Tracks();

  // Decrease the height of each track by one "unit"
  foreach (TrackOutput* t, all_tracks) {
    t->SetTrackHeight(qMax(t->GetTrackHeight() - t->GetTrackHeightIncrement(), t->GetTrackHeightMinimum()));
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

void TimelineWidget::RippleEditTo(Timeline::MovementMode mode, bool insert_gaps)
{
  rational playhead_time = Timecode::timestamp_to_time(playhead_, timebase());

  rational closest_point_to_playhead;
  if (mode == Timeline::kTrimIn) {
    closest_point_to_playhead = 0;
  } else {
    closest_point_to_playhead = RATIONAL_MAX;
  }

  foreach (TrackOutput* track, timeline_node_->Tracks()) {
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

  foreach (TrackOutput* track, timeline_node_->Tracks()) {
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
  return views_.at(ref.type())->view()->GetTrackY(ref.index());
}

int TimelineWidget::GetTrackHeight(const TrackReference &ref)
{
  return views_.at(ref.type())->view()->GetTrackHeight(ref.index());
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

  foreach (TimelineAndTrackView* view, views_) {
    view->view()->SetScale(scale_);
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
  timecode_label_->SetValue(timestamp);
}

void TimelineWidget::UpdateTimelineLength(const rational &length)
{
  foreach (TimelineAndTrackView* view, views_) {
    view->view()->SetEndTime(length);
  }
}

TimelineWidget::Tool *TimelineWidget::GetActiveTool()
{
  return tools_.at(Core::instance()->tool());
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
  if (timeline_node_) {
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
  if (timeline_node_ != nullptr && active_tool_ != nullptr) {
    active_tool_->MouseRelease(event);
    active_tool_ = nullptr;
  }
}

void TimelineWidget::ViewMouseDoubleClicked(TimelineViewMouseEvent *event)
{
  if (timeline_node_ != nullptr && active_tool_ != nullptr) {
    active_tool_->MouseDoubleClick(event);
    active_tool_ = nullptr;
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
    views_.at(track.type())->view()->scene()->addItem(item);

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

void TimelineWidget::BlockChanged()
{
  TimelineViewRect* rect = block_items_[static_cast<Block*>(sender())];

  if (rect != nullptr) {
    rect->UpdateRect();
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

  //QList<TimelineViewBlockItem*> selected = GetSelectedBlocks();
  QAction* speed_duration_action = menu.addAction(tr("Speed/Duration"));
  connect(speed_duration_action, SIGNAL(triggered(bool)), this, SLOT(ShowSpeedDurationDialog()));

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

  SpeedDurationDialog speed_diag(timebase(), selected_clips, this);
  speed_diag.exec();
}

void TimelineWidget::AddGhost(TimelineViewGhostItem *ghost)
{
  ghost->SetScale(scale_);
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

void TimelineWidget::EndRubberBandSelect(bool select_links)
{
  MoveRubberBandSelect(select_links);
  rubberband_.hide();
  rubberband_now_selected_.clear();
}
