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

#include "timelinewidget.h"

#include <cfloat>
#include <QSplitter>
#include <QVBoxLayout>
#include <QtMath>

#include "core.h"
#include "common/range.h"
#include "common/timecodefunctions.h"
#include "dialog/sequence/sequence.h"
#include "node/block/transition/transition.h"
#include "tool/add.h"
#include "tool/beam.h"
#include "tool/edit.h"
#include "tool/pointer.h"
#include "tool/razor.h"
#include "tool/ripple.h"
#include "tool/rolling.h"
#include "tool/slide.h"
#include "tool/slip.h"
#include "tool/transition.h"
#include "tool/zoom.h"
#include "tool/tool.h"
#include "trackview/trackview.h"
#include "widget/menu/menu.h"
#include "widget/menu/menushared.h"
#include "widget/nodeview/nodeviewundo.h"

namespace olive {

TimelineWidget::TimelineWidget(QWidget *parent) :
  TimeBasedWidget(true, true, parent),
  rubberband_(QRubberBand::Rectangle, this),
  active_tool_(nullptr),
  use_audio_time_units_(false)
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
  ruler()->SetSnapService(this);

  // Create list of TimelineViews - these MUST correspond to the ViewType enum

  view_splitter_ = new QSplitter(Qt::Vertical);
  view_splitter_->setChildrenCollapsible(false);
  vert_layout->addWidget(view_splitter_);

  // Video view
  views_.append(new TimelineAndTrackView(Qt::AlignBottom));

  // Audio view
  views_.append(new TimelineAndTrackView(Qt::AlignTop));

  // Create tools
  tools_.resize(olive::Tool::kCount);
  tools_.fill(nullptr);

  tools_.replace(olive::Tool::kPointer, new PointerTool(this));
  tools_.replace(olive::Tool::kEdit, new EditTool(this));
  tools_.replace(olive::Tool::kRipple, new RippleTool(this));
  tools_.replace(olive::Tool::kRolling, new RollingTool(this));
  tools_.replace(olive::Tool::kRazor, new RazorTool(this));
  tools_.replace(olive::Tool::kSlip, new SlipTool(this));
  tools_.replace(olive::Tool::kSlide, new SlideTool(this));
  tools_.replace(olive::Tool::kZoom, new ZoomTool(this));
  tools_.replace(olive::Tool::kTransition, new TransitionTool(this));
  //tools_.replace(olive::Tool::kRecord, new PointerTool(this));  FIXME: Implement
  tools_.replace(olive::Tool::kAdd, new AddTool(this));

  import_tool_ = new ImportTool(this);

  // We add this to the list to make deleting all tools easier, it should never get accessed through the list under
  // normal circumstances (but *technically* its index would be Tool::kCount)
  tools_.append(import_tool_);

  // Global scrollbar
  connect(scrollbar(), &QScrollBar::valueChanged, ruler(), &TimeRuler::SetScroll);
  connect(views_.first()->view()->horizontalScrollBar(), &QScrollBar::rangeChanged, scrollbar(), &QScrollBar::setRange);
  vert_layout->addWidget(scrollbar());

  connect(ruler(), &TimeRuler::TimeChanged, this, &TimelineWidget::SetViewTimestamp);

  foreach (TimelineAndTrackView* tview, views_) {
    TimelineView* view = tview->view();

    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->SetSnapService(this);
    view->SetSelectionList(&selections_);
    view->SetGhostList(&ghost_items_);

    view_splitter_->addWidget(tview);

    ConnectTimelineView(view);

    connect(view->horizontalScrollBar(), &QScrollBar::valueChanged, ruler(), &TimeRuler::SetScroll);
    connect(view, &TimelineView::ScaleChanged, this, &TimelineWidget::SetScale);
    connect(view, &TimelineView::TimeChanged, this, &TimelineWidget::ViewTimestampChanged);
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
  view_splitter_->setSizes({INT_MAX, INT_MAX});

  // FIXME: Magic number
  SetScale(90.0);

  SetMaximumScale(TimelineViewBase::kMaximumScale);
  SetAutoSetTimebase(false);

  connect(Core::instance(), &Core::ToolChanged, this, &TimelineWidget::ToolChanged);
}

TimelineWidget::~TimelineWidget()
{
  // Ensure no blocks are selected before any child widgets are destroyed (prevents corrupt ViewSelectionChanged() signal)
  Clear();

  qDeleteAll(tools_);
}

void TimelineWidget::Clear()
{
  // Delete all items
  for (auto iterator=block_items_.begin(); iterator!=block_items_.end(); iterator++) {
    delete iterator.value();
  }
  block_items_.clear();

  // Emit that we've deselected any selected blocks
  SignalDeselectedAllBlocks();

  // Set null timebase
  SetTimebase(0);
}

void TimelineWidget::TimebaseChangedEvent(const rational &timebase)
{
  TimeBasedWidget::TimebaseChangedEvent(timebase);

  timecode_label_->SetTimebase(timebase);

  timecode_label_->setVisible(!timebase.isNull());

  QMap<Block*, TimelineViewBlockItem*>::const_iterator iterator;

  for (iterator=block_items_.begin();iterator!=block_items_.end();iterator++) {
    if (iterator.value()) {
      iterator.value()->SetTimebase(timebase);
    }
  }

  UpdateViewTimebases();
}

void TimelineWidget::resizeEvent(QResizeEvent *event)
{
  TimeBasedWidget::resizeEvent(event);

  // Update timecode label size
  UpdateTimecodeWidthFromSplitters(views_.first()->splitter());
}

void TimelineWidget::TimeChangedEvent(const int64_t& timestamp)
{
  SetViewTimestamp(timestamp);

  timecode_label_->SetValue(timestamp);
}

void TimelineWidget::ScaleChangedEvent(const double &scale)
{
  TimeBasedWidget::ScaleChangedEvent(scale);

  QMap<Block*, TimelineViewBlockItem*>::const_iterator iterator;

  for (iterator=block_items_.begin();iterator!=block_items_.end();iterator++) {
    if (iterator.value()) {
      iterator.value()->SetScale(scale);
    }
  }

  foreach (TimelineAndTrackView* view, views_) {
    view->view()->SetScale(scale);
  }
}

void TimelineWidget::ConnectNodeInternal(ViewerOutput *n)
{
  connect(n, &ViewerOutput::BlockAdded, this, &TimelineWidget::AddBlock);
  connect(n, &ViewerOutput::BlockRemoved, this, &TimelineWidget::RemoveBlock);
  connect(n, &ViewerOutput::TrackAdded, this, &TimelineWidget::AddTrack);
  connect(n, &ViewerOutput::TrackRemoved, this, &TimelineWidget::RemoveTrack);
  connect(n, &ViewerOutput::TimebaseChanged, this, &TimelineWidget::SetTimebase);
  connect(n, &ViewerOutput::TrackHeightChanged, this, &TimelineWidget::TrackHeightChanged);

  ruler()->SetPlaybackCache(n->video_frame_cache());

  SetTimebase(n->video_params().time_base());

  for (int i=0;i<views_.size();i++) {
    Timeline::TrackType track_type = static_cast<Timeline::TrackType>(i);
    TimelineView* view = views_.at(i)->view();
    TrackList* track_list = n->track_list(track_type);
    TrackView* track_view = views_.at(i)->track_view();

    track_view->ConnectTrackList(track_list);
    view->ConnectTrackList(track_list);

    // Defer to the track to make all the block UI items necessary
    foreach (TrackOutput* track, n->track_list(track_type)->GetTracks()) {
      AddTrack(track, track_type);
    }
  }
}

void TimelineWidget::DisconnectNodeInternal(ViewerOutput *n)
{
  disconnect(n, &ViewerOutput::BlockAdded, this, &TimelineWidget::AddBlock);
  disconnect(n, &ViewerOutput::BlockRemoved, this, &TimelineWidget::RemoveBlock);
  disconnect(n, &ViewerOutput::TrackAdded, this, &TimelineWidget::AddTrack);
  disconnect(n, &ViewerOutput::TrackRemoved, this, &TimelineWidget::RemoveTrack);
  disconnect(n, &ViewerOutput::TimebaseChanged, this, &TimelineWidget::SetTimebase);
  disconnect(n, &ViewerOutput::TrackHeightChanged, this, &TimelineWidget::TrackHeightChanged);

  DeselectAll();

  foreach (TrackOutput* track, n->GetTracks()) {
    RemoveTrack(track);
  }

  ruler()->SetPlaybackCache(nullptr);

  SetTimebase(0);

  Clear();

  foreach (TimelineAndTrackView* tview, views_) {
    tview->track_view()->DisconnectTrackList();
    tview->view()->ConnectTrackList(nullptr);
  }
}

void TimelineWidget::CopyNodesToClipboardInternal(QXmlStreamWriter *writer, void* userdata)
{
  // Cache the earliest in point so all copied clips have a "relative" in point that can be pasted anywhere
  QList<TimelineViewBlockItem*>& selected = *static_cast<QList<TimelineViewBlockItem*>*>(userdata);
  rational earliest_in = RATIONAL_MAX;

  foreach (TimelineViewBlockItem* item, selected) {
    Block* block = item->block();

    earliest_in = qMin(earliest_in, block->in());
  }

  foreach (TimelineViewBlockItem* item, selected) {
    Block* block = item->block();

    writer->writeStartElement(QStringLiteral("block"));

    writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(block)));
    writer->writeAttribute(QStringLiteral("in"), (block->in() - earliest_in).toString());

    TrackOutput* track = TrackOutput::TrackFromBlock(block);

    if (track) {
      writer->writeAttribute(QStringLiteral("tracktype"), QString::number(track->track_type()));
      writer->writeAttribute(QStringLiteral("trackindex"), QString::number(track->Index()));
    }

    writer->writeEndElement();
  }
}

void TimelineWidget::PasteNodesFromClipboardInternal(QXmlStreamReader *reader, XMLNodeData& xml_node_data, void *userdata)
{
  QList<BlockPasteData>& paste_data = *static_cast<QList<BlockPasteData>*>(userdata);

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("block")) {
      BlockPasteData bpd;

      foreach (QXmlStreamAttribute attr, reader->attributes()) {
        if (attr.name() == QStringLiteral("ptr")) {
          bpd.block = static_cast<Block*>(xml_node_data.node_ptrs.value(attr.value().toULongLong()));
        } else if (attr.name() == QStringLiteral("in")) {
          bpd.in = rational::fromString(attr.value().toString());
        } else if (attr.name() == QStringLiteral("tracktype")) {
          bpd.track_type = static_cast<Timeline::TrackType>(attr.value().toInt());
        } else if (attr.name() == QStringLiteral("trackindex")) {
          bpd.track_index = attr.value().toInt();
        }
      }

      paste_data.append(bpd);

      reader->skipCurrentElement();
    }
  }
}

rational TimelineWidget::GetToolTipTimebase() const
{
  if (GetConnectedNode() && use_audio_time_units_) {
    return GetConnectedNode()->audio_params().time_base();
  }
  return timebase();
}

void TimelineWidget::SelectAll()
{
  QVector<Block*> newly_selected_blocks;

  for (auto it=block_items_.cbegin(); it!=block_items_.cend(); it++) {
    if (!selected_blocks_.contains(it.key())) {
      newly_selected_blocks.append(it.key());
      AddSelection(it.key()->range(), it.value()->Track());
    }
  }

  SignalSelectedBlocks(newly_selected_blocks, false);
}

void TimelineWidget::DeselectAll()
{
  // Clear selections
  selections_.clear();

  // Update all viewports
  UpdateViewports();

  // Clear list and emit signal
  SignalDeselectedAllBlocks();
}

void TimelineWidget::RippleToIn()
{
  RippleTo(Timeline::kTrimIn);
}

void TimelineWidget::RippleToOut()
{
  RippleTo(Timeline::kTrimOut);
}

void TimelineWidget::EditToIn()
{
  EditTo(Timeline::kTrimIn);
}

void TimelineWidget::EditToOut()
{
  EditTo(Timeline::kTrimOut);
}

void TimelineWidget::SplitAtPlayhead()
{
  if (!GetConnectedNode()) {
    return;
  }

  rational playhead_time = Timecode::timestamp_to_time(GetTimestamp(), timebase());

  QVector<TimelineViewBlockItem *> selected_blocks = GetSelectedBlocks();

  // Prioritize blocks that are selected and overlap the playhead
  QVector<Block*> blocks_to_split;
  QVector<bool> block_is_selected;

  bool some_blocks_are_selected = false;

  // Get all blocks at the playhead
  foreach (TrackOutput* track, GetConnectedNode()->GetTracks()) {
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

void TimelineWidget::ReplaceBlocksWithGaps(const QVector<Block *> &blocks,
                                            bool remove_from_graph,
                                            QUndoCommand *command)
{
  foreach (Block* b, blocks) {
    if (b->type() == Block::kGap) {
      // No point in replacing a gap with a gap, and TrackReplaceBlockWithGapCommand will clear
      // up any extraneous gaps
      continue;
    }

    TrackOutput* original_track = TrackOutput::TrackFromBlock(b);

    new TrackReplaceBlockWithGapCommand(original_track, b, command);

    if (remove_from_graph) {
      new NodeRemoveWithExclusiveDeps(static_cast<NodeGraph*>(b->parent()), b, command);
    }
  }
}

void TimelineWidget::DeleteSelected(bool ripple)
{
  QVector<TimelineViewBlockItem *> selected_list = GetSelectedBlocks();
  QVector<Block*> blocks_to_delete;

  foreach (TimelineViewBlockItem* item, selected_list) {
    Block* b = item->block();

    blocks_to_delete.append(b);
  }

  // No-op if nothing is selected
  if (blocks_to_delete.isEmpty()) {
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  QVector<Block*> clips_to_delete;
  QVector<TransitionBlock*> transitions_to_delete;

  foreach (Block* b, blocks_to_delete) {
    if (b->type() == Block::kClip) {
      clips_to_delete.append(b);
    } else if (b->type() == Block::kTransition) {
      transitions_to_delete.append(static_cast<TransitionBlock*>(b));
    }
  }

  // For transitions, remove them but extend their attached blocks to fill their place
  foreach (TransitionBlock* transition, transitions_to_delete) {
    new TransitionRemoveCommand(TrackOutput::TrackFromBlock(transition),
                                transition,
                                command);

    new NodeRemoveWithExclusiveDeps(static_cast<NodeGraph*>(GetConnectedNode()->parent()),
                                    transition,
                                    command);
  }

  // Replace clips with gaps (effectively deleting them)
  ReplaceBlocksWithGaps(clips_to_delete, true, command);

  // Remove all selections
  new TimelineSetSelectionsCommand(this, TimelineWidgetSelections(), GetSelections(), command);

  // Insert ripple command now that it's all cleaned up gaps
  if (ripple) {
    TimeRangeList range_list;

    foreach (Block* b, blocks_to_delete) {
      range_list.insert(TimeRange(b->in(), b->out()));
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

  QVector<TrackOutput*> all_tracks = GetConnectedNode()->GetTracks();

  // Increase the height of each track by one "unit"
  foreach (TrackOutput* t, all_tracks) {
    t->SetTrackHeight(t->GetTrackHeight() + TrackOutput::kTrackHeightInterval);
  }
}

void TimelineWidget::DecreaseTrackHeight()
{
  if (!GetConnectedNode()) {
    return;
  }

  QVector<TrackOutput*> all_tracks = GetConnectedNode()->GetTracks();

  // Decrease the height of each track by one "unit"
  foreach (TrackOutput* t, all_tracks) {
    t->SetTrackHeight(qMax(t->GetTrackHeight() - TrackOutput::kTrackHeightInterval, TrackOutput::kTrackHeightMinimum));
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
  QVector<TimelineViewBlockItem*> sel = GetSelectedBlocks();

  QVector<Block*> blocks;
  bool link = true;

  foreach (TimelineViewBlockItem* item, sel) {
    // Only clips can be linked
    if (item->block()->type() != Block::kClip) {
      continue;
    }

    // Prioritize unlinking, if any block has links, assume we're unlinking
    if (link && item->block()->HasLinks()) {
      link = false;
    }

    blocks.append(item->block());
  }

  if (blocks.isEmpty()) {
    return;
  }

  Core::instance()->undo_stack()->push(new BlockLinkManyCommand(blocks, link));
}

void TimelineWidget::CopySelected(bool cut)
{
  if (!GetConnectedNode()) {
    return;
  }

  QVector<TimelineViewBlockItem*> selected = GetSelectedBlocks();

  if (selected.isEmpty()) {
    return;
  }

  QVector<Node*> selected_nodes;

  foreach (TimelineViewBlockItem* item, selected) {
    Node* block = item->block();

    selected_nodes.append(block);

    QVector<Node*> deps = block->GetDependencies();

    foreach (Node* d, deps) {
      if (!selected_nodes.contains(d)) {
        selected_nodes.append(d);
      }
    }
  }

  CopyNodesToClipboard(selected_nodes, &selected);

  if (cut) {
    DeleteSelected();
  }
}

void TimelineWidget::Paste(bool insert)
{
  if (!GetConnectedNode()) {
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  QVector<BlockPasteData> paste_data;
  QVector<Node*> pasted = PasteNodesFromClipboard(static_cast<Sequence*>(GetConnectedNode()->parent()), command, &paste_data);

  rational paste_start = GetTime();

  if (insert) {
    rational paste_end = GetTime();

    foreach (const BlockPasteData& bpd, paste_data) {
      paste_end = qMax(paste_end, paste_start + bpd.in + bpd.block->length());
    }

    if (paste_end != paste_start) {
      InsertGapsAt(paste_start, paste_end - paste_start, command);
    }
  }

  foreach (const BlockPasteData& bpd, paste_data) {
    qDebug() << "Placing" << bpd.block;
    new TrackPlaceBlockCommand(GetConnectedNode()->track_list(bpd.track_type),
                               bpd.track_index,
                               bpd.block,
                               paste_start + bpd.in,
                               command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void TimelineWidget::DeleteInToOut(bool ripple)
{
  if (!GetConnectedNode()
      || !GetConnectedTimelinePoints()
      || !GetConnectedTimelinePoints()->workarea()->enabled()) {
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  if (ripple) {

    new TimelineRippleRemoveAreaCommand(GetConnectedNode(),
                                        GetConnectedTimelinePoints()->workarea()->in(),
                                        GetConnectedTimelinePoints()->workarea()->out(),
                                        command);

  } else {
    QVector<TrackOutput*> unlocked_tracks = GetConnectedNode()->GetUnlockedTracks();

    foreach (TrackOutput* track, unlocked_tracks) {
      GapBlock* gap = new GapBlock();

      gap->set_length_and_media_out(GetConnectedTimelinePoints()->workarea()->length());

      new NodeAddCommand(static_cast<NodeGraph*>(track->parent()),
                         gap,
                         command);

      new TrackPlaceBlockCommand(GetConnectedNode()->track_list(track->track_type()),
                                 track->Index(),
                                 gap,
                                 GetConnectedTimelinePoints()->workarea()->in(),
                                 command);
    }
  }

  // Clear workarea after this
  new WorkareaSetEnabledCommand(GetTimelinePointsProject(),
                                GetConnectedTimelinePoints(),
                                false,
                                command);

  if (ripple) {
    SetTimeAndSignal(Timecode::time_to_timestamp(GetConnectedTimelinePoints()->workarea()->in(),
                                                 timebase()));
  }

  Core::instance()->undo_stack()->push(command);
}

void TimelineWidget::ToggleSelectedEnabled()
{
  QVector<TimelineViewBlockItem*> items = GetSelectedBlocks();

  if (items.isEmpty()) {
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  foreach (TimelineViewBlockItem* i, items) {
    new BlockEnableDisableCommand(i->block(),
                                  !i->block()->is_enabled(),
                                  command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

QVector<TimelineViewBlockItem *> TimelineWidget::GetSelectedBlocks()
{
  QVector<TimelineViewBlockItem *> list(selected_blocks_.size());

  for (int i=0; i<selected_blocks_.size(); i++) {
    list[i] = block_items_.value(selected_blocks_.at(i));
  }

  return list;
}

void TimelineWidget::InsertGapsAt(const rational &earliest_point, const rational &insert_length, QUndoCommand *command)
{
  for (int i=0;i<Timeline::kTrackTypeCount;i++) {
    new TrackListInsertGaps(GetConnectedNode()->track_list(static_cast<Timeline::TrackType>(i)),
                            earliest_point,
                            insert_length,
                            command);
  }
}

TrackOutput *TimelineWidget::GetTrackFromReference(const TrackReference &ref)
{
  return GetConnectedNode()->track_list(ref.type())->GetTrackAt(ref.index());
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

  HideSnaps();
}

TimelineTool *TimelineWidget::GetActiveTool()
{
  return tools_.at(Core::instance()->tool());
}

void TimelineWidget::ViewMousePressed(TimelineViewMouseEvent *event)
{
  active_tool_ = GetActiveTool();

  if (GetConnectedNode() && active_tool_ != nullptr) {
    active_tool_->MousePress(event);
    UpdateViewports();
  }

  if (event->GetButton() != Qt::LeftButton) {
    // Suspend tool immediately if the cursor isn't the primary button
    active_tool_->MouseRelease(event);
    UpdateViewports();
    active_tool_ = nullptr;
  }
}

void TimelineWidget::ViewMouseMoved(TimelineViewMouseEvent *event)
{
  if (GetConnectedNode()) {
    if (active_tool_) {
      active_tool_->MouseMove(event);
      UpdateViewports();
    } else {
      // Mouse is not down, attempt a hover event
      TimelineTool* hover_tool = GetActiveTool();

      if (hover_tool) {
        hover_tool->HoverMove(event);
        UpdateViewports();
      }
    }
  }
}

void TimelineWidget::ViewMouseReleased(TimelineViewMouseEvent *event)
{
  if (GetConnectedNode() && active_tool_ != nullptr) {
    active_tool_->MouseRelease(event);
    UpdateViewports();
    active_tool_ = nullptr;
  }
}

void TimelineWidget::ViewMouseDoubleClicked(TimelineViewMouseEvent *event)
{
  if (GetConnectedNode()) {
    GetActiveTool()->MouseDoubleClick(event);
    UpdateViewports();
  }
}

void TimelineWidget::ViewDragEntered(TimelineViewMouseEvent *event)
{
  import_tool_->DragEnter(event);
  UpdateViewports();
}

void TimelineWidget::ViewDragMoved(TimelineViewMouseEvent *event)
{
  import_tool_->DragMove(event);
  UpdateViewports();
}

void TimelineWidget::ViewDragLeft(QDragLeaveEvent *event)
{
  import_tool_->DragLeave(event);
  UpdateViewports();
}

void TimelineWidget::ViewDragDropped(TimelineViewMouseEvent *event)
{
  import_tool_->DragDrop(event);
  UpdateViewports();
}

void TimelineWidget::AddBlock(Block *block, TrackReference track)
{
  // Set up clip with view parameters (clip item will automatically size its rect accordingly)
  TimelineViewBlockItem* item = block_items_.value(block);

  if (!item) {

    // Add to list of clip items that can be iterated through
    item = new TimelineViewBlockItem(block);
    block_items_.insert(block, item);

    // Set scale parameters
    item->SetScale(GetScale());
    item->SetTimebase(timebase());
    item->SetYCoords(GetTrackY(track), GetTrackHeight(track));
    item->SetTrack(track);

    // Add item to graphics scene
    views_.at(track.type())->view()->scene()->addItem(item);

    connect(block, &Block::Refreshed, this, &TimelineWidget::BlockRefreshed);
    connect(block, &Block::LinksChanged, this, &TimelineWidget::BlockUpdated);
    connect(block, &Block::LabelChanged, this, &TimelineWidget::BlockUpdated);
    connect(block, &Block::EnabledChanged, this, &TimelineWidget::BlockUpdated);

  } else if (item->Track() != track) {

    item->SetYCoords(GetTrackY(track), GetTrackHeight(track));
    item->SetTrack(track);

  }
}

void TimelineWidget::RemoveBlock(const QList<Block *> &blocks)
{
  QVector<Block*> deselect_blocks;

  foreach (Block* b, blocks) {
    // Disconnect all signals
    disconnect(b, &Block::Refreshed, this, &TimelineWidget::BlockRefreshed);
    disconnect(b, &Block::LinksChanged, this, &TimelineWidget::BlockUpdated);
    disconnect(b, &Block::LabelChanged, this, &TimelineWidget::BlockUpdated);
    disconnect(b, &Block::EnabledChanged, this, &TimelineWidget::BlockUpdated);

    // Take item from map
    TimelineViewBlockItem* item = block_items_.take(b);

    // If selected, deselect it
    int select_index = selected_blocks_.indexOf(b);
    if (select_index > -1) {
      selected_blocks_.removeAt(select_index);
      deselect_blocks.append(b);
      RemoveSelection(item);
    }

    // Finally, delete item
    delete item;
  }

  if (!deselect_blocks.isEmpty()) {
    // We already removed the blocks from selected_blocks_, so we can signal directly rather than
    // through
    emit BlocksDeselected(deselect_blocks);
  }
}

void TimelineWidget::AddTrack(TrackOutput *track, Timeline::TrackType type)
{
  foreach (Block* b, track->Blocks()) {
    AddBlock(b, TrackReference(type, track->Index()));
  }

  connect(track, &TrackOutput::IndexChanged, this, &TimelineWidget::TrackIndexChanged);
  connect(track, &TrackOutput::PreviewChanged, this, &TimelineWidget::TrackPreviewUpdated);
}

void TimelineWidget::RemoveTrack(TrackOutput *track)
{
  disconnect(track, &TrackOutput::IndexChanged, this, &TimelineWidget::TrackIndexChanged);
  disconnect(track, &TrackOutput::PreviewChanged, this, &TimelineWidget::TrackPreviewUpdated);

  RemoveBlock(track->Blocks());
}

void TimelineWidget::TrackIndexChanged()
{
  TrackOutput* track = static_cast<TrackOutput*>(sender());
  TrackReference ref(track->track_type(), track->Index());

  foreach (Block* b, track->Blocks()) {
    TimelineViewBlockItem* item = block_items_.value(b);

    item->SetYCoords(GetTrackY(ref), GetTrackHeight(ref));
    item->SetTrack(ref);
  }
}

void TimelineWidget::BlockRefreshed()
{
  TimelineViewRect* rect = block_items_.value(static_cast<Block*>(sender()));

  if (rect) {
    rect->UpdateRect();
  }
}

void TimelineWidget::BlockUpdated()
{
  TimelineViewRect* rect = block_items_.value(static_cast<Block*>(sender()));

  if (rect) {
    rect->update();
  }
}

void TimelineWidget::TrackPreviewUpdated()
{
  QMap<Block*, TimelineViewBlockItem*>::const_iterator i;

  TrackOutput* track = static_cast<TrackOutput*>(sender());
  TrackReference track_ref(track->track_type(), track->Index());

  for (i=block_items_.constBegin(); i!=block_items_.constEnd(); i++) {
    if (i.value()->Track() == track_ref) {
      i.value()->update();
    }
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

  QVector<TimelineViewBlockItem*> selected = GetSelectedBlocks();

  if (!selected.isEmpty()) {
    MenuShared::instance()->AddItemsForEditMenu(&menu, true);

    menu.addSeparator();

    QAction* properties_action = menu.addAction(tr("Properties"));
    connect(properties_action, &QAction::triggered, this, [this](){
      QVector<TimelineViewBlockItem*> block_items = GetSelectedBlocks();
      QVector<Node*> nodes;

      foreach (TimelineViewBlockItem* i, block_items) {
        nodes.append(i->block());
      }

      Core::instance()->LabelNodes(nodes);
    });
  }

  if (selected.isEmpty()) {

    QAction* toggle_audio_units = menu.addAction(tr("Use Audio Time Units"));
    toggle_audio_units->setCheckable(true);
    toggle_audio_units->setChecked(use_audio_time_units_);
    connect(toggle_audio_units, &QAction::triggered, this, &TimelineWidget::SetUseAudioTimeUnits);

    menu.addSeparator();

    QAction* properties_action = menu.addAction(tr("Properties"));
    connect(properties_action, &QAction::triggered, this, &TimelineWidget::ShowSequenceDialog);
  }

  menu.exec(QCursor::pos());
}

void TimelineWidget::DeferredScrollAction()
{
  scrollbar()->setValue(deferred_scroll_value_);
}

void TimelineWidget::ShowSequenceDialog()
{
  if (!GetConnectedNode()) {
    return;
  }

  SequenceDialog sd(static_cast<Sequence*>(GetConnectedNode()->parent()), SequenceDialog::kExisting, this);
  sd.exec();
}

void TimelineWidget::SetUseAudioTimeUnits(bool use)
{
  use_audio_time_units_ = use;

  // Update timebases
  UpdateViewTimebases();

  // Force update of the viewer timestamps
  SetViewTimestamp(GetTimestamp());
}

void TimelineWidget::SetViewTimestamp(const int64_t &ts)
{
  for (int i=0;i<views_.size();i++) {
    TimelineAndTrackView* view = views_.at(i);

    if (use_audio_time_units_ && i == Timeline::kTrackTypeAudio) {
      view->view()->SetTime(Timecode::rescale_timestamp(ts,
                                                        timebase(),
                                                        GetConnectedNode()->audio_params().time_base()));
    } else {
      view->view()->SetTime(ts);
    }
  }
}

void TimelineWidget::ViewTimestampChanged(int64_t ts)
{
  if (use_audio_time_units_ && sender() == views_.at(Timeline::kTrackTypeAudio)) {
    ts = Timecode::rescale_timestamp(ts,
                                     GetConnectedNode()->audio_params().time_base(),
                                     timebase());
  }

  // Update all other views
  SetViewTimestamp(ts);

  ruler()->SetTime(ts);
  emit TimeChanged(ts);
}

void TimelineWidget::ToolChanged()
{
  HideSnaps();
}

void TimelineWidget::AddGhost(TimelineViewGhostItem *ghost)
{
  ghost_items_.append(ghost);

  UpdateViewports(ghost->GetTrack().type());
}

void TimelineWidget::UpdateViewTimebases()
{
  for (int i=0;i<views_.size();i++) {
    TimelineAndTrackView* view = views_.at(i);

    if (use_audio_time_units_ && i == Timeline::kTrackTypeAudio) {
      view->view()->SetTimebase(GetConnectedNode()->audio_params().time_base());
    } else {
      view->view()->SetTimebase(timebase());
    }
  }
}

void TimelineWidget::SetViewBeamCursor(const TimelineCoordinate &coord)
{
  foreach (TimelineAndTrackView* tview, views_) {
    tview->view()->SetBeamCursor(coord);
  }
}

void TimelineWidget::SetBlockLinksSelected(Block* block, bool selected)
{
  TimelineViewBlockItem* link_item;

  foreach (Block* link, block->linked_clips()) {
    link_item = block_items_.value(link);

    if (link_item) {
      if (selected) {
        AddSelection(link_item);
      } else {
        RemoveSelection(link_item);
      }
    }
  }
}

void TimelineWidget::QueueScroll(int value)
{
  // (using a hacky singleShot so the scroll occurs after the scene and its scrollbars have updated)
  deferred_scroll_value_ = value;

  QTimer::singleShot(0, this, &TimelineWidget::DeferredScrollAction);
}

TimelineView *TimelineWidget::GetFirstTimelineView()
{
  return views_.first()->view();
}

const QRect& TimelineWidget::GetRubberBandGeometry() const
{
  return rubberband_.geometry();
}

void TimelineWidget::SignalSelectedBlocks(QVector<Block *> input, bool filter)
{
  if (input.isEmpty()) {
    return;
  }

  if (filter) {
    // If filtering, remove all the blocks that are already selected
    for (int i=0; i<input.size(); i++) {
      Block* b = input.at(i);

      if (selected_blocks_.contains(b)) {
        input.removeAt(i);
        i--;
      }
    }
  }

  selected_blocks_.append(input);

  emit BlocksSelected(input);
}

void TimelineWidget::SignalDeselectedBlocks(const QVector<Block *> &deselected_blocks)
{
  if (deselected_blocks.isEmpty()) {
    return;
  }

  foreach (Block* b, deselected_blocks) {
    selected_blocks_.removeOne(b);
  }

  emit BlocksDeselected(deselected_blocks);
}

void TimelineWidget::SignalDeselectedAllBlocks()
{
  if (!selected_blocks_.isEmpty()) {
    emit BlocksDeselected(selected_blocks_);
    selected_blocks_.clear();
  }
}

QVector<Timeline::EditToInfo> TimelineWidget::GetEditToInfo(const rational& playhead_time,
                                                            Timeline::MovementMode mode)
{
  // Get list of unlocked tracks
  QVector<TrackOutput*> tracks = GetConnectedNode()->GetUnlockedTracks();

  // Create list to cache nearest times and the blocks at this point
  QVector<Timeline::EditToInfo> info_list(tracks.size());

  for (int i=0;i<tracks.size();i++) {
    Timeline::EditToInfo info;

    TrackOutput* track = tracks.at(i);
    info.track = track;

    Block* b;

    // Determine what block is at this time (for "trim in", we want to catch blocks that start at
    // the time, for "trim out" we don't)
    if (mode == Timeline::kTrimIn) {
      b = track->NearestBlockBeforeOrAt(playhead_time);
    } else {
      b = track->NearestBlockBefore(playhead_time);
    }

    // If we have a block here, cache how close it is to the track
    if (b) {
      rational this_track_closest_point;

      if (mode == Timeline::kTrimIn) {
        this_track_closest_point = b->in();
      } else {
        this_track_closest_point = b->out();
      }

      info.nearest_time = this_track_closest_point;
    }

    info.nearest_block = b;

    info_list[i] = info;
  }

  return info_list;
}

void TimelineWidget::RippleTo(Timeline::MovementMode mode)
{
  rational playhead_time = GetTime();

  QVector<Timeline::EditToInfo> tracks = GetEditToInfo(playhead_time, mode);

  if (tracks.isEmpty()) {
    return;
  }

  // Find each track's nearest point and determine the overall timeline's nearest point
  rational closest_point_to_playhead = (mode == Timeline::kTrimIn) ? rational() : RATIONAL_MAX;

  foreach (const Timeline::EditToInfo& info, tracks) {
    if (info.nearest_block) {
      if (mode == Timeline::kTrimIn) {
        closest_point_to_playhead = qMax(info.nearest_time, closest_point_to_playhead);
      } else {
        closest_point_to_playhead = qMin(info.nearest_time, closest_point_to_playhead);
      }
    }
  }

  // If we're not inserting gaps and the edit point is right on the nearest in point, we enter a
  // single-frame mode where we remove one frame only
  if (closest_point_to_playhead == playhead_time) {
    if (mode == Timeline::kTrimIn) {
      playhead_time += timebase();
    } else {
      playhead_time -= timebase();
    }
  }

  // For standard rippling, we can cache here the region that will be rippled out
  rational in_ripple = qMin(closest_point_to_playhead, playhead_time);
  rational out_ripple = qMax(closest_point_to_playhead, playhead_time);

  TimelineRippleRemoveAreaCommand* c = new TimelineRippleRemoveAreaCommand(GetConnectedNode(),
                                                                           in_ripple,
                                                                           out_ripple);

  Core::instance()->undo_stack()->push(c);

  // If we rippled, ump to where new cut is if applicable
  if (mode == Timeline::kTrimIn) {
    SetTimeAndSignal(Timecode::time_to_timestamp(closest_point_to_playhead, timebase()));
  } else if (mode == Timeline::kTrimOut && closest_point_to_playhead == GetTime()) {
    SetTimeAndSignal(Timecode::time_to_timestamp(playhead_time, timebase()));
  }
}

void TimelineWidget::EditTo(Timeline::MovementMode mode)
{
  const rational playhead_time = GetTime();

  // Get list of unlocked tracks
  QVector<Timeline::EditToInfo> tracks = GetEditToInfo(playhead_time, mode);

  if (tracks.isEmpty()) {
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  foreach (const Timeline::EditToInfo& info, tracks) {
    if (info.nearest_block
        && info.nearest_block->type() != Block::kGap
        && info.nearest_time != playhead_time) {
      rational new_len;

      if (mode == Timeline::kTrimIn) {
        new_len = playhead_time - info.nearest_time;
      } else {
        new_len = info.nearest_time - playhead_time;
      }
      new_len = info.nearest_block->length() - new_len;

      new BlockTrimCommand(info.track,
                           info.nearest_block,
                           new_len,
                           mode,
                           command);
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void TimelineWidget::ShowSnap(const QList<rational> &times)
{
  foreach (TimelineAndTrackView* tview, views_) {
    tview->view()->EnableSnap(times);
  }
}

void TimelineWidget::UpdateViewports(const Timeline::TrackType &type)
{
  if (type == Timeline::kTrackTypeNone) {
    foreach (TimelineAndTrackView* tview, views_) {
      tview->view()->viewport()->update();
    }
  } else {
    views_.at(type)->view()->viewport()->update();
  }
}

void TimelineWidget::HideSnaps()
{
  foreach (TimelineAndTrackView* tview, views_) {
    tview->view()->DisableSnap();
  }
}

QByteArray TimelineWidget::SaveSplitterState() const
{
  return view_splitter_->saveState();
}

void TimelineWidget::RestoreSplitterState(const QByteArray &state)
{
  view_splitter_->restoreState(state);
}

void TimelineWidget::StartRubberBandSelect(bool enable_selecting, bool select_links)
{
  drag_origin_ = QCursor::pos();
  rubberband_.show();

  // We don't touch any blocks that are already selected. If you want these to be deselected by
  // default, call DeselectAll() before calling StartRubberBandSelect()
  rubberband_old_selections_ = selections_;

  MoveRubberBandSelect(enable_selecting, select_links);
}

void TimelineWidget::MoveRubberBandSelect(bool enable_selecting, bool select_links)
{
  QPoint rubberband_now = QCursor::pos();

  rubberband_.setGeometry(QRect(mapFromGlobal(drag_origin_), mapFromGlobal(rubberband_now)).normalized());

  if (!enable_selecting) {
    return;
  }

  QList<QGraphicsItem*> items_in_rubberband;

  // Determine all items in the rubberband
  foreach (TimelineAndTrackView* tview, views_) {
    TimelineView* view = tview->view();

    // Map global mouse coordinates to viewport
    QRect mapped_rect(view->viewport()->mapFromGlobal(drag_origin_),
                      view->viewport()->mapFromGlobal(rubberband_now));

    // Normalize and get items in rect
    QList<QGraphicsItem*> rubberband_items = view->items(mapped_rect.normalized());

    items_in_rubberband.append(rubberband_items);
  }

  // Reset selection to whatever it was before
  SetSelections(rubberband_old_selections_);

  // Add any blocks in rubberband
  rubberband_now_selected_.clear();

  foreach (QGraphicsItem* item, items_in_rubberband) {
    TimelineViewBlockItem* block_item = dynamic_cast<TimelineViewBlockItem*>(item);

    if (block_item) {
      Block* b = block_item->block();

      if (b->type() == Block::kGap) {
        continue;
      }

      TrackOutput* t = GetTrackFromReference(block_item->Track());
      if (t && t->IsLocked()) {
        continue;
      }

      if (!rubberband_now_selected_.contains(b)) {
        AddSelection(block_item);
        rubberband_now_selected_.append(b);
      }

      if (select_links) {
        foreach (Block* link, b->linked_clips()) {
          if (!rubberband_now_selected_.contains(link)) {
            AddSelection(block_items_.value(link));
            rubberband_now_selected_.append(link);
          }
        }
      }
    }
  }
}

void TimelineWidget::EndRubberBandSelect()
{
  rubberband_.hide();

  // Emit any blocks that were newly selected
  SignalSelectedBlocks(rubberband_now_selected_);

  rubberband_now_selected_.clear();
  rubberband_old_selections_.clear();
}

void TimelineWidget::AddSelection(const TimeRange &time, const TrackReference &track)
{
  selections_[track].insert(time);

  UpdateViewports(track.type());
}

void TimelineWidget::AddSelection(TimelineViewBlockItem *item)
{
  AddSelection(item->block()->range(), item->Track());
}

void TimelineWidget::RemoveSelection(const TimeRange &time, const TrackReference &track)
{
  selections_[track].remove(time);

  UpdateViewports(track.type());
}

void TimelineWidget::RemoveSelection(TimelineViewBlockItem *item)
{
  RemoveSelection(item->block()->range(), item->Track());
}

void TimelineWidget::SetSelections(const TimelineWidgetSelections &s)
{
  selections_ = s;

  UpdateViewports();
}

TimelineViewBlockItem *TimelineWidget::GetItemAtScenePos(const TimelineCoordinate& coord)
{
  for (auto it=block_items_.cbegin(); it!=block_items_.cend(); it++) {
    Block* b = it.key();
    TimelineViewBlockItem* item = it.value();

    if (b->in() <= coord.GetFrame()
        && b->out() > coord.GetFrame()
        && item->Track() == coord.GetTrack()) {
      return item;
    }
  }

  return nullptr;
}

struct SnapData {
  rational time;
  rational movement;
};

QList<SnapData> AttemptSnap(const QList<double>& screen_pt,
                            double compare_pt,
                            const QList<rational>& start_times,
                            const rational& compare_time) {
  const qreal kSnapRange = 10; // FIXME: Hardcoded number

  QList<SnapData> snap_data;

  for (int i=0;i<screen_pt.size();i++) {
    // Attempt snapping to clip out point
    if (InRange(screen_pt.at(i), compare_pt, kSnapRange)) {
      snap_data.append({compare_time, compare_time - start_times.at(i)});
    }
  }

  return snap_data;
}

bool TimelineWidget::SnapPoint(QList<rational> start_times, rational* movement, int snap_points)
{
  QList<double> screen_pt;

  foreach (const rational& s, start_times) {
    screen_pt.append(TimeToScene(s + *movement));
  }

  QList<SnapData> potential_snaps;

  if (snap_points & kSnapToPlayhead) {
    rational playhead_abs_time = GetTime();
    qreal playhead_pos = TimeToScene(playhead_abs_time);
    potential_snaps.append(AttemptSnap(screen_pt, playhead_pos, start_times, playhead_abs_time));
  }

  if (snap_points & kSnapToClips) {
    QMap<Block*, TimelineViewBlockItem*>::const_iterator i;

    for (i=block_items_.constBegin(); i!=block_items_.constEnd(); i++) {
      TimelineViewBlockItem* item = i.value();

      if (item) {
        qreal rect_left = item->x();
        qreal rect_right = rect_left + item->rect().width();

        // Attempt snapping to clip in point
        potential_snaps.append(AttemptSnap(screen_pt, rect_left, start_times, item->block()->in()));

        // Attempt snapping to clip out point
        potential_snaps.append(AttemptSnap(screen_pt, rect_right, start_times, item->block()->out()));
      }
    }
  }

  if ((snap_points & kSnapToMarkers) && GetConnectedTimelinePoints()) {
    foreach (TimelineMarker* m, GetConnectedTimelinePoints()->markers()->list()) {
      qreal marker_pos = TimeToScene(m->time().in());
      potential_snaps.append(AttemptSnap(screen_pt, marker_pos, start_times, m->time().in()));

      if (m->time().in() != m->time().out()) {
        marker_pos = TimeToScene(m->time().out());
        potential_snaps.append(AttemptSnap(screen_pt, marker_pos, start_times, m->time().out()));
      }
    }
  }

  if (potential_snaps.isEmpty()) {
    HideSnaps();
    return false;
  }

  int closest_snap = 0;
  rational closest_diff = qAbs(potential_snaps.at(0).movement - *movement);

  // Determine which snap point was the closest
  for (int i=1; i<potential_snaps.size(); i++) {
    rational this_diff = qAbs(potential_snaps.at(i).movement - *movement);

    if (this_diff < closest_diff) {
      closest_snap = i;
      closest_diff = this_diff;
    }
  }

  *movement = potential_snaps.at(closest_snap).movement;

  // Find all points at this movement
  QList<rational> snap_times;
  foreach (const SnapData& d, potential_snaps) {
    if (d.movement == *movement) {
      snap_times.append(d.time);
    }
  }

  ShowSnap(snap_times);

  return true;
}

}
