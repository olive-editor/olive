/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include "dialog/sequence/sequence.h"
#include "dialog/speedduration/speeddurationdialog.h"
#include "node/block/transition/transition.h"
#include "node/nodeundo.h"
#include "node/project/serializer/serializer.h"
#include "task/project/import/import.h"
#include "timeline/timelineundogeneral.h"
#include "timeline/timelineundopointer.h"
#include "timeline/timelineundoripple.h"
#include "timeline/timelineundoworkarea.h"
#include "tool/add.h"
#include "tool/beam.h"
#include "tool/edit.h"
#include "tool/pointer.h"
#include "tool/razor.h"
#include "tool/record.h"
#include "tool/ripple.h"
#include "tool/rolling.h"
#include "tool/slide.h"
#include "tool/slip.h"
#include "tool/trackselect.h"
#include "tool/transition.h"
#include "tool/zoom.h"
#include "tool/tool.h"
#include "trackview/trackview.h"
#include "widget/menu/menu.h"
#include "widget/menu/menushared.h"
#include "widget/nodeparamview/nodeparamview.h"
#include "widget/timeruler/timeruler.h"

namespace olive {

#define super TimeBasedWidget

TimelineWidget::TimelineWidget(QWidget *parent) :
  super(true, true, parent),
  rubberband_(QRubberBand::Rectangle, this),
  active_tool_(nullptr),
  use_audio_time_units_(false),
  subtitle_show_command_(nullptr),
  subtitle_tentative_track_(nullptr)
{
  QVBoxLayout* vert_layout = new QVBoxLayout(this);
  vert_layout->setSpacing(0);
  vert_layout->setContentsMargins(0, 0, 0, 0);

  QHBoxLayout* ruler_and_time_layout = new QHBoxLayout();
  vert_layout->addLayout(ruler_and_time_layout);

  timecode_label_ = new RationalSlider();
  timecode_label_->SetAlignment(Qt::AlignCenter);
  timecode_label_->SetDisplayType(RationalSlider::kTime);
  timecode_label_->setVisible(false);
  timecode_label_->SetMinimum(0);
  ruler_and_time_layout->addWidget(timecode_label_);

  ruler_and_time_layout->addWidget(ruler());

  ruler()->setFocusPolicy(Qt::TabFocus);
  QWidget::setTabOrder(ruler(), timecode_label_);

  // Create list of TimelineViews - these MUST correspond to the ViewType enum

  view_splitter_ = new QSplitter(Qt::Vertical);
  vert_layout->addWidget(view_splitter_);

  // Video view
  views_.append(AddTimelineAndTrackView(Qt::AlignBottom));

  // Audio view
  views_.append(AddTimelineAndTrackView(Qt::AlignTop));

  // Subtitle view
  views_.append(AddTimelineAndTrackView(Qt::AlignTop));

  // Create tools
  tools_.resize(olive::Tool::kCount);
  tools_.fill(nullptr);

  tools_.replace(olive::Tool::kPointer, new PointerTool(this));
  tools_.replace(olive::Tool::kTrackSelect, new TrackSelectTool(this));
  tools_.replace(olive::Tool::kEdit, new EditTool(this));
  tools_.replace(olive::Tool::kRipple, new RippleTool(this));
  tools_.replace(olive::Tool::kRolling, new RollingTool(this));
  tools_.replace(olive::Tool::kRazor, new RazorTool(this));
  tools_.replace(olive::Tool::kSlip, new SlipTool(this));
  tools_.replace(olive::Tool::kSlide, new SlideTool(this));
  tools_.replace(olive::Tool::kZoom, new ZoomTool(this));
  tools_.replace(olive::Tool::kTransition, new TransitionTool(this));
  tools_.replace(olive::Tool::kRecord, new RecordTool(this));
  tools_.replace(olive::Tool::kAdd, new AddTool(this));

  import_tool_ = new ImportTool(this);

  // We add this to the list to make deleting all tools easier, it should never get accessed through the list under
  // normal circumstances (but *technically* its index would be Tool::kCount)
  tools_.append(import_tool_);

  // Global scrollbar
  connect(views_.first()->view()->horizontalScrollBar(), &QScrollBar::rangeChanged, scrollbar(), &QScrollBar::setRange);
  vert_layout->addWidget(scrollbar());

  foreach (TimelineAndTrackView* tview, views_) {
    TimelineView* view = tview->view();

    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->SetSnapService(this);
    view->SetSelectionList(&selections_);
    view->SetGhostList(&ghost_items_);

    view_splitter_->addWidget(tview);

    ConnectTimelineView(view);

    connect(view, &TimelineView::customContextMenuRequested, this, &TimelineWidget::ShowContextMenu);

    connect(view, &TimelineView::MousePressed, this, &TimelineWidget::ViewMousePressed);
    connect(view, &TimelineView::MouseMoved, this, &TimelineWidget::ViewMouseMoved);
    connect(view, &TimelineView::MouseReleased, this, &TimelineWidget::ViewMouseReleased);
    connect(view, &TimelineView::MouseDoubleClicked, this, &TimelineWidget::ViewMouseDoubleClicked);
    connect(view, &TimelineView::DragEntered, this, &TimelineWidget::ViewDragEntered);
    connect(view, &TimelineView::DragMoved, this, &TimelineWidget::ViewDragMoved);
    connect(view, &TimelineView::DragLeft, this, &TimelineWidget::ViewDragLeft);
    connect(view, &TimelineView::DragDropped, this, &TimelineWidget::ViewDragDropped);

    connect(tview->splitter(), &QSplitter::splitterMoved, this, &TimelineWidget::UpdateHorizontalSplitters);
  }

  // Split viewer 50/50
  QList<int> view_sizes;
  view_sizes.reserve(views_.size());
  view_sizes.append(height()/2);  // Video
  view_sizes.append(height()/2);  // Audio
  view_sizes.append(0);           // Subtitle (hidden by default)
  view_splitter_->setSizes(view_sizes);

  // Video and audio are not collapsible, subtitle is
  view_splitter_->setCollapsible(Track::kVideo, false);
  view_splitter_->setCollapsible(Track::kAudio, false);
  view_splitter_->setCollapsible(Track::kSubtitle, true);

  // FIXME: Magic number
  SetScale(90.0);

  SetAutoSetTimebase(false);

  connect(Core::instance(), &Core::ToolChanged, this, &TimelineWidget::ToolChanged);
  connect(Core::instance(), &Core::AddableObjectChanged, this, &TimelineWidget::AddableObjectChanged);

  signal_block_change_timer_ = new QTimer(this);
  signal_block_change_timer_->setInterval(1);
  signal_block_change_timer_->setSingleShot(true);
  connect(signal_block_change_timer_, &QTimer::timeout, this, [this]{
    signal_block_change_timer_->stop();

    if (OLIVE_CONFIG("SelectAlsoSeeks").toBool()) {
      rational start = RATIONAL_MAX;
      for (Block *b : selected_blocks_) {
        start = std::min(start, b->in());
      }
      if (start != RATIONAL_MAX) {
        GetConnectedNode()->SetPlayhead(start);
      }
    }

    emit BlockSelectionChanged(selected_blocks_);
  });
}

TimelineWidget::~TimelineWidget()
{
  // Ensure no blocks are selected before any child widgets are destroyed (prevents corrupt ViewSelectionChanged() signal)
  ConnectViewerNode(nullptr);

  Clear();

  qDeleteAll(tools_);

  delete subtitle_show_command_;
}

void TimelineWidget::Clear()
{
  // Emit that we've deselected any selected blocks
  SignalDeselectedAllBlocks();

  // Set null timebase
  SetTimebase(0);
}

void TimelineWidget::TimebaseChangedEvent(const rational &timebase)
{
  super::TimebaseChangedEvent(timebase);

  timecode_label_->SetTimebase(timebase);

  timecode_label_->setVisible(!timebase.isNull());

  UpdateViewTimebases();
}

void TimelineWidget::resizeEvent(QResizeEvent *event)
{
  super::resizeEvent(event);

  // Update timecode label size
  UpdateTimecodeWidthFromSplitters(views_.first()->splitter());
}

void TimelineWidget::TimeChangedEvent(const rational &t)
{
  if (OLIVE_CONFIG("SeekAlsoSelects").toBool()) {
    TimelineWidgetSelections sels;

    QVector<Block*> new_blocks;

    for (auto it=sequence()->GetTracks().cbegin(); it!=sequence()->GetTracks().cend(); it++) {
      Track *track = *it;
      if (track->IsLocked()) {
        continue;
      }

      Block *b = track->VisibleBlockAtTime(sequence()->GetPlayhead());
      if (!b || dynamic_cast<GapBlock*>(b)) {
        continue;
      }

      new_blocks.push_back(b);
      sels[track->ToReference()].insert(b->range());
    }

    if (selected_blocks_ != new_blocks) {
      selected_blocks_ = new_blocks;
      SetSelections(sels, false);
      SignalBlockSelectionChange();
    }
  }
}

void TimelineWidget::ScaleChangedEvent(const double &scale)
{
  super::ScaleChangedEvent(scale);

  foreach (TimelineAndTrackView* view, views_) {
    view->view()->SetScale(scale);
  }

  if (rubberband_.isVisible()) {
    QMetaObject::invokeMethod(this, &TimelineWidget::ForceUpdateRubberBand, Qt::QueuedConnection);
  }
}

void TimelineWidget::ConnectNodeEvent(ViewerOutput *n)
{
  Sequence* s = static_cast<Sequence*>(n);

  connect(s, &Sequence::TrackAdded, this, &TimelineWidget::AddTrack);
  connect(s, &Sequence::TrackRemoved, this, &TimelineWidget::RemoveTrack);
  connect(s, &Sequence::FrameRateChanged, this, &TimelineWidget::FrameRateChanged);
  connect(s, &Sequence::SampleRateChanged, this, &TimelineWidget::SampleRateChanged);

  connect(timecode_label_, &RationalSlider::ValueChanged, s, &Sequence::SetPlayhead);
  connect(s, &Sequence::PlayheadChanged, timecode_label_, &RationalSlider::SetValue);
  timecode_label_->SetValue(s->GetPlayhead());

  ruler()->SetPlaybackCache(n->video_frame_cache());

  SetTimebase(n->GetVideoParams().frame_rate_as_time_base());

  for (int i=0;i<views_.size();i++) {
    Track::Type track_type = static_cast<Track::Type>(i);
    TimelineView* view = views_.at(i)->view();
    TrackList* track_list = s->track_list(track_type);
    TrackView* track_view = views_.at(i)->track_view();

    track_view->ConnectTrackList(track_list);
    view->ConnectTrackList(track_list);

    // Defer to the track to make all the block UI items necessary
    const QVector<Track*> tracks = s->track_list(track_type)->GetTracks();
    foreach (Track* track, tracks) {
      AddTrack(track);
    }
  }
}

void TimelineWidget::DisconnectNodeEvent(ViewerOutput *n)
{
  Sequence* s = static_cast<Sequence*>(n);

  disconnect(s, &Sequence::TrackAdded, this, &TimelineWidget::AddTrack);
  disconnect(s, &Sequence::TrackRemoved, this, &TimelineWidget::RemoveTrack);
  disconnect(s, &Sequence::FrameRateChanged, this, &TimelineWidget::FrameRateChanged);
  disconnect(s, &Sequence::SampleRateChanged, this, &TimelineWidget::SampleRateChanged);

  disconnect(timecode_label_, &RationalSlider::ValueChanged, s, &Sequence::SetPlayhead);

  DeselectAll();

  foreach (Track* track, s->GetTracks()) {
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

void TimelineWidget::SendCatchUpScrollEvent()
{
  super::SendCatchUpScrollEvent();

  if (rubberband_.isVisible()) {
    this->ForceUpdateRubberBand();
  }
}

void TimelineWidget::SelectAll()
{
  QVector<Block*> newly_selected_blocks;

  foreach (Block* block, added_blocks_) {
    if (!selected_blocks_.contains(block)) {
      newly_selected_blocks.append(block);
      AddSelection(block);
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

  const rational &playhead_time = GetConnectedNode()->GetPlayhead();

  QVector<Block*> selected_blocks = GetSelectedBlocks();

  // Prioritize blocks that are selected and overlap the playhead
  QVector<Block*> blocks_to_split;
  QVector<bool> block_is_selected;

  bool some_blocks_are_selected = false;

  // Get all blocks at the playhead
  foreach (Track* track, sequence()->GetTracks()) {
    if (track->IsLocked()) {
      continue;
    }

    Block* b = track->BlockContainingTime(playhead_time);

    if (dynamic_cast<ClipBlock*>(b)) {
      bool selected = false;

      // See if this block is selected
      foreach (Block* item, selected_blocks) {
        if (item == b) {
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
                                           MultiUndoCommand *command,
                                           bool handle_transitions)
{
  foreach (Block* b, blocks) {
    if (dynamic_cast<GapBlock*>(b)) {
      // No point in replacing a gap with a gap, and TrackReplaceBlockWithGapCommand will clear
      // up any extraneous gaps
      continue;
    }

    Track* original_track = b->track();

    command->add_child(new TrackReplaceBlockWithGapCommand(original_track, b, handle_transitions));

    if (remove_from_graph) {
      command->add_child(new NodeRemoveWithExclusiveDependenciesAndDisconnect(b));
    }
  }
}

void TimelineWidget::DeleteSelected(bool ripple)
{
  if (ruler()->HasItemsSelected()) {
    ruler()->DeleteSelected();
    return;
  }

  QVector<Block*> selected_list = GetSelectedBlocks();

  // No-op if nothing is selected
  if (selected_list.isEmpty()) {
    return;
  }

  QVector<Block*> clips_to_delete;
  QVector<TransitionBlock*> transitions_to_delete;

  bool all_gaps = true;

  foreach (Block* b, selected_list) {
    if (!dynamic_cast<GapBlock*>(b)) {
      all_gaps = false;
    }

    if (dynamic_cast<ClipBlock*>(b)) {
      clips_to_delete.append(b);
    } else if (dynamic_cast<TransitionBlock*>(b)) {
      transitions_to_delete.append(static_cast<TransitionBlock*>(b));
    }
  }

  if (all_gaps) {
    ripple = true;
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  // Remove all selections
  command->add_child(new SetSelectionsCommand(this, TimelineWidgetSelections(), GetSelections()));

  // For transitions, remove them but extend their attached blocks to fill their place
  foreach (TransitionBlock* transition, transitions_to_delete) {
    TransitionRemoveCommand *trc = new TransitionRemoveCommand(transition, true);

    // Perform the transition removal now so that replacing blocks with gaps below won't get confused
    trc->redo_now();

    command->add_child(trc);
  }

  // Replace clips with gaps (effectively deleting them)
  ReplaceBlocksWithGaps(clips_to_delete, true, command, false);

  // Insert ripple command now that it's all cleaned up gaps
  TimelineRippleDeleteGapsAtRegionsCommand *ripple_command = nullptr;
  rational new_playhead = RATIONAL_MAX;
  if (ripple) {
    TimelineRippleDeleteGapsAtRegionsCommand::RangeList range_list;

    foreach (Block* b, selected_list) {
      range_list.append({b->track(), b->range()});
      new_playhead = qMin(new_playhead, b->in());
    }

    ripple_command = new TimelineRippleDeleteGapsAtRegionsCommand(sequence(), range_list);
    command->add_child(ripple_command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  // Ensures any current drag operations are cancelled
  ClearGhosts();

  if (ripple_command && ripple_command->HasCommands() && new_playhead != RATIONAL_MAX) {
    GetConnectedNode()->SetPlayhead(new_playhead);
  }
}

void TimelineWidget::IncreaseTrackHeight()
{
  if (!GetConnectedNode()) {
    return;
  }

  // Increase the height of each track by one "unit"
  foreach (Track* t, sequence()->GetTracks()) {
    t->SetTrackHeight(t->GetTrackHeight() + Track::kTrackHeightInterval);
  }
}

void TimelineWidget::DecreaseTrackHeight()
{
  if (!GetConnectedNode()) {
    return;
  }

  // Decrease the height of each track by one "unit"
  foreach (Track* t, sequence()->GetTracks()) {
    t->SetTrackHeight(qMax(t->GetTrackHeight() - Track::kTrackHeightInterval, Track::kTrackHeightMinimum));
  }
}

void TimelineWidget::InsertFootageAtPlayhead(const QVector<ViewerOutput*>& footage)
{
  auto command = new MultiUndoCommand();
  import_tool_->PlaceAt(footage, GetConnectedNode()->GetPlayhead(), true, command);
  Core::instance()->undo_stack()->push(command);
}

void TimelineWidget::OverwriteFootageAtPlayhead(const QVector<ViewerOutput *> &footage)
{
  auto command = new MultiUndoCommand();
  import_tool_->PlaceAt(footage, GetConnectedNode()->GetPlayhead(), false, command);
  Core::instance()->undo_stack()->push(command);
}

void TimelineWidget::ToggleLinksOnSelected()
{
  QVector<Node*> blocks;
  bool link = true;

  foreach (Block* item, GetSelectedBlocks()) {
    // Only clips can be linked
    if (!dynamic_cast<ClipBlock*>(item)) {
      continue;
    }

    // Prioritize unlinking, if any block has links, assume we're unlinking
    if (link && item->HasLinks()) {
      link = false;
    }

    blocks.append(item);
  }

  if (blocks.isEmpty()) {
    return;
  }

  Core::instance()->undo_stack()->push(new NodeLinkManyCommand(blocks, link));
}

void TimelineWidget::AddDefaultTransitionsToSelected()
{
  QVector<ClipBlock*> blocks;

  foreach (Block* item, GetSelectedBlocks()) {
    // Only clips can be linked
    if (ClipBlock *clip = dynamic_cast<ClipBlock*>(item)) {
      blocks.append(clip);
    }
  }

  if (!blocks.isEmpty()) {
    Core::instance()->undo_stack()->push(new TimelineAddDefaultTransitionCommand(blocks, timebase()));
  }
}

bool TimelineWidget::CopySelected(bool cut)
{
  if (super::CopySelected(cut)) {
    return true;
  }

  if (!GetConnectedNode() || selected_blocks_.isEmpty()) {
    return false;
  }

  QVector<Node*> selected_nodes;

  foreach (Block* block, selected_blocks_) {
    selected_nodes.append(block);

    QVector<Node*> deps = block->GetDependencies();

    foreach (Node* d, deps) {
      if (!selected_nodes.contains(d)) {
        selected_nodes.append(d);
      }
    }
  }

  ProjectSerializer::SaveData sdata(selected_nodes.first()->project());
  sdata.SetOnlySerializeNodesAndResolveGroups(selected_nodes);

  // Cache the earliest in point so all copied clips have a "relative" in point that can be pasted anywhere
  rational earliest_in = RATIONAL_MAX;
  ProjectSerializer::SerializedProperties properties;

  foreach (Block* block, selected_blocks_) {
    earliest_in = qMin(earliest_in, block->in());
  }

  foreach (Block* block, selected_blocks_) {
    properties[block][QStringLiteral("in")] = QString::fromStdString((block->in() - earliest_in).toString());
    properties[block][QStringLiteral("track")] = block->track()->ToReference().ToString();
  }

  sdata.SetProperties(properties);

  ProjectSerializer::Copy(sdata, QStringLiteral("timeline"));

  if (cut) {
    DeleteSelected();
  }

  return true;
}

bool TimelineWidget::Paste()
{
  // TimeRuler gets first chance (markers, etc.)
  if (super::Paste()) {
    return true;
  }

  // Ensure we have a connected node
  if (!GetConnectedNode()) {
    return false;
  }

  // Attempt regular clip pasting
  if (PasteInternal(false)) {
    return true;
  }

  // Give last chance to NodeParamView
  return NodeParamView::Paste(this, std::bind(&TimelineWidget::GenerateExistingPasteMap, this, std::placeholders::_1));
}

void TimelineWidget::PasteInsert()
{
  PasteInternal(true);
}

void TimelineWidget::DeleteInToOut(bool ripple)
{
  if (!GetConnectedNode()
      || !GetConnectedNode()->GetWorkArea()->enabled()) {
    return;
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  if (ripple) {

    command->add_child(new TimelineRippleRemoveAreaCommand(
                         sequence(),
                         GetConnectedNode()->GetWorkArea()->in(),
                         GetConnectedNode()->GetWorkArea()->out()));

  } else {
    QVector<Track*> unlocked_tracks = sequence()->GetUnlockedTracks();

    foreach (Track* track, unlocked_tracks) {
      GapBlock* gap = new GapBlock();

      gap->set_length_and_media_out(GetConnectedNode()->GetWorkArea()->length());

      command->add_child(new NodeAddCommand(static_cast<Project*>(track->parent()),
                                            gap));

      command->add_child(new TrackPlaceBlockCommand(sequence()->track_list(track->type()),
                                                    track->Index(),
                                                    gap,
                                                    GetConnectedNode()->GetWorkArea()->in()));
    }
  }

  // Clear workarea after this
  command->add_child(new WorkareaSetEnabledCommand(GetConnectedNode()->project(),
                                                   GetConnectedNode()->GetWorkArea(),
                                                   false));

  if (ripple) {
    GetConnectedNode()->SetPlayhead(GetConnectedNode()->GetWorkArea()->in());
  }

  Core::instance()->undo_stack()->push(command);
}

void TimelineWidget::ToggleSelectedEnabled()
{
  QVector<Block*> items = GetSelectedBlocks();

  if (items.isEmpty()) {
    return;
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  foreach (Block* i, items) {
    command->add_child(new BlockEnableDisableCommand(i,
                                                     !i->is_enabled()));
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void TimelineWidget::SetColorLabel(int index)
{
  MultiUndoCommand *command = new MultiUndoCommand();

  foreach (Block* b, selected_blocks_) {
    command->add_child(new NodeOverrideColorCommand(b, index));
  }

  Core::instance()->undo_stack()->push(command);
}

void TimelineWidget::NudgeLeft()
{
  if (GetConnectedNode()) {
    NudgeInternal(-timebase());
  }
}

void TimelineWidget::NudgeRight()
{
  if (GetConnectedNode()) {
    NudgeInternal(timebase());
  }
}

void TimelineWidget::MoveInToPlayhead()
{
  MoveToPlayheadInternal(false);
}

void TimelineWidget::MoveOutToPlayhead()
{
  MoveToPlayheadInternal(true);
}

void TimelineWidget::ShowSpeedDurationDialogForSelectedClips()
{
  QVector<ClipBlock*> clips;

  foreach (Block *b, selected_blocks_) {
    ClipBlock *c = dynamic_cast<ClipBlock*>(b);
    if (c) {
      clips.append(c);
    }
  }

  if (!clips.isEmpty()) {
    SpeedDurationDialog sdd(clips, timebase(), this);
    sdd.exec();
  }
}

void TimelineWidget::RecordingCallback(const QString &filename, const TimeRange &time, const Track::Reference &track)
{
  ProjectImportTask task(GetConnectedNode()->project()->root(), {filename});
  task.Start();

  auto subimport_command = task.GetCommand();

  if (task.GetImportedFootage().empty()) {
    qCritical() << "Failed to import recorded audio file" << filename;
    delete subimport_command;
  } else {
    subimport_command->redo_now();

    auto import_command = new MultiUndoCommand();
    import_command->add_child(subimport_command);

    import_tool_->PlaceAt({task.GetImportedFootage().front()}, time.in(), false, import_command, track.index());
    Core::instance()->undo_stack()->pushIfHasChildren(import_command);
  }
}

void TimelineWidget::EnableRecordingOverlay(const TimelineCoordinate &coord)
{
  foreach (TimelineAndTrackView* tview, views_) {
    tview->view()->EnableRecordingOverlay(coord);
  }
}

void TimelineWidget::DisableRecordingOverlay()
{
  foreach (TimelineAndTrackView* tview, views_) {
    tview->view()->DisableRecordingOverlay();
  }
}

void TimelineWidget::AddTentativeSubtitleTrack()
{
  if (!subtitle_show_command_) {
    // Determine if we need to do anything
    QList<int> sz = view_splitter_->sizes();
    bool should_adjust_splitter = (sz[Track::kSubtitle] == 0);
    bool should_add_sub_track = (sequence() && sequence()->track_list(Track::kSubtitle)->GetTrackCount() == 0);

    if (should_adjust_splitter || should_add_sub_track) {
      // Create command
      subtitle_show_command_ = new MultiUndoCommand();

      if (should_adjust_splitter) {
        sz[Track::kSubtitle] = height() / Track::kCount;
        subtitle_show_command_->add_child(new SetSplitterSizesCommand(view_splitter_, sz));
      }

      if (should_add_sub_track) {
        TimelineAddTrackCommand *track_add_cmd = new TimelineAddTrackCommand(sequence()->track_list(Track::kSubtitle));
        subtitle_tentative_track_ = track_add_cmd->track();
        subtitle_show_command_->add_child(track_add_cmd);
      }

      subtitle_show_command_->redo_now();
    }
  }
}

void TimelineWidget::NestSelectedClips()
{
  if (!GetConnectedNode()) {
    return;
  }

  QVector<Block*> blocks = this->selected_blocks_;
  if (blocks.empty()) {
    return;
  }

  QVector<Track::Reference> tracks(blocks.size());
  QVector<TimeRange> times(blocks.size());
  QVector<int> track_offset(Track::kCount, INT_MAX);
  rational start_time = RATIONAL_MAX;
  rational end_time = RATIONAL_MIN;
  for (int i=0; i<blocks.size(); i++) {
    Block *b = blocks.at(i);

    Track::Reference tf = b->track()->ToReference();;
    tracks[i] = tf;
    times[i] = b->range();

    int &to = track_offset[tf.type()];
    to = std::min(to, tf.index());

    start_time = std::min(start_time, b->in());
    end_time = std::max(end_time, b->out());
  }

  auto move_to_nest_command = new MultiUndoCommand();

  // Remove blocks from this sequence
  ReplaceBlocksWithGaps(blocks, false, move_to_nest_command);

  // Create new sequence
  Project *project = this->GetConnectedNode()->project();
  Sequence *nest = Core::CreateNewSequenceForProject(tr("Nested Sequence %1"), project);
  nest->SetVideoParams(GetConnectedNode()->GetVideoParams());
  nest->SetAudioParams(GetConnectedNode()->GetAudioParams());
  move_to_nest_command->add_child(new NodeAddCommand(project, nest));

  // Add to same folder
  move_to_nest_command->add_child(new FolderAddChild(this->GetConnectedNode()->folder(), nest));

  // Place blocks in new sequence
  for (int i=0; i<blocks.size(); i++) {
    Block *b = blocks.at(i);

    const TimeRange &range = times.at(i);
    Track::Reference track = tracks.at(i);

    move_to_nest_command->add_child(new TrackPlaceBlockCommand(nest->track_list(track.type()),
                                                  track.index() - track_offset.at(track.type()),
                                                  b, range.in() - start_time));
  }

  // Do this command now, because we later do checks and actions that rely on these having been done
  move_to_nest_command->redo_now();

  auto meta_command = new MultiUndoCommand();
  meta_command->add_child(move_to_nest_command);

  // Find first free track index
  bool empty = false;
  int index = -1;
  while (!empty) {
    index++;
    empty = true;
    for (int i=0; i<Track::kCount; i++) {
      if (track_offset.at(i) == INT_MAX) {
        // No clips on this track
        continue;
      }

      TrackList *list = sequence()->track_list(static_cast<Track::Type>(i));
      if (index < list->GetTrackCount() && !list->GetTrackAt(index)->IsRangeFree(TimeRange(start_time, end_time))) {
        empty = false;
        break;
      }
    }
  }

  // Place new sequence in this sequence
  import_tool_->PlaceAt({nest}, start_time, false, meta_command, index);

  Core::instance()->undo_stack()->push(meta_command);
}

void TimelineWidget::ClearTentativeSubtitleTrack()
{
  if (subtitle_show_command_) {
    subtitle_show_command_->undo_now();
    delete subtitle_show_command_;
    subtitle_show_command_ = nullptr;
    subtitle_tentative_track_ = nullptr;
  }
}

void TimelineWidget::InsertGapsAt(const rational &earliest_point, const rational &insert_length, MultiUndoCommand *command)
{
  for (int i=0;i<Track::kCount;i++) {
    command->add_child(new TrackListInsertGaps(sequence()->track_list(static_cast<Track::Type>(i)),
                                               earliest_point,
                                               insert_length));
  }
}

Track *TimelineWidget::GetTrackFromReference(const Track::Reference &ref) const
{
  return sequence()->track_list(ref.type())->GetTrackAt(ref.index());
}

int TimelineWidget::GetTrackY(const Track::Reference &ref)
{
  return views_.at(ref.type())->view()->GetTrackY(ref.index());
}

int TimelineWidget::GetTrackHeight(const Track::Reference &ref)
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

      SetCatchUpScrollValue(event->GetScreenPos().x());
    } else {
      // Mouse is not down, attempt a hover event
      TimelineTool* hover_tool = GetActiveTool();

      if (hover_tool) {
        hover_tool->HoverMove(event);
      }
    }
  }
}


void TimelineWidget::ViewMouseReleased(TimelineViewMouseEvent *event)
{
  StopCatchUpScrollTimer();

  if (active_tool_) {
    if (GetConnectedNode()) {
      active_tool_->MouseRelease(event);
      UpdateViewports();
    }

    active_tool_ = nullptr;
  }
}

void TimelineWidget::ViewMouseDoubleClicked(TimelineViewMouseEvent *event)
{
  // kHand tool will return nullptr
  if (!GetActiveTool()) {
    // Only kHand should return a nullptr
    Q_ASSERT(Core::instance()->tool() == olive::Tool::kHand);
    return;
  }
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

  SetCatchUpScrollValue(event->GetScreenPos().x());
}

void TimelineWidget::ViewDragLeft(QDragLeaveEvent *event)
{
  StopCatchUpScrollTimer();

  import_tool_->DragLeave(event);
  UpdateViewports();
}

void TimelineWidget::ViewDragDropped(TimelineViewMouseEvent *event)
{
  StopCatchUpScrollTimer();

  import_tool_->DragDrop(event);
  UpdateViewports();
}

void TimelineWidget::AddBlock(Block *block)
{
  // Set up clip with view parameters (clip item will automatically size its rect accordingly)
  if (!added_blocks_.contains(block)) {
    connect(block, &Block::LinksChanged, this, &TimelineWidget::BlockUpdated);
    connect(block, &Block::LabelChanged, this, &TimelineWidget::BlockUpdated);
    connect(block, &Block::ColorChanged, this, &TimelineWidget::BlockUpdated);
    connect(block, &Block::EnabledChanged, this, &TimelineWidget::BlockUpdated);
    connect(block, &Block::PreviewChanged, this, &TimelineWidget::BlockUpdated);

    added_blocks_.append(block);

    if (selections_[block->track()->ToReference()].contains(block->range()) && !selected_blocks_.contains(block)) {
      selected_blocks_.append(block);
    }
  }
}

void TimelineWidget::RemoveBlock(Block *block)
{
  // Disconnect all signals
  disconnect(block, &Block::LinksChanged, this, &TimelineWidget::BlockUpdated);
  disconnect(block, &Block::LabelChanged, this, &TimelineWidget::BlockUpdated);
  disconnect(block, &Block::ColorChanged, this, &TimelineWidget::BlockUpdated);
  disconnect(block, &Block::EnabledChanged, this, &TimelineWidget::BlockUpdated);
  disconnect(block, &Block::PreviewChanged, this, &TimelineWidget::BlockUpdated);

  // Take item from map
  added_blocks_.removeOne(block);

  // If selected, deselect it
  int select_index = selected_blocks_.indexOf(block);
  if (select_index > -1) {
    selected_blocks_.removeAt(select_index);
    RemoveSelection(block);

    SignalBlockSelectionChange();
  }
}

void TimelineWidget::AddTrack(Track *track)
{
  foreach (Block* b, track->Blocks()) {
    AddBlock(b);
  }

  connect(track, &Track::IndexChanged, this, &TimelineWidget::TrackUpdated);
  connect(track, &Track::IndexChanged, this, &TimelineWidget::TrackIndexChanged);
  connect(track, &Track::BlocksRefreshed, this, &TimelineWidget::TrackUpdated);
  connect(track, &Track::TrackHeightChanged, this, &TimelineWidget::TrackUpdated);
  connect(track, &Track::BlockAdded, this, &TimelineWidget::AddBlock);
  connect(track, &Track::BlockRemoved, this, &TimelineWidget::RemoveBlock);
}

void TimelineWidget::RemoveTrack(Track *track)
{
  disconnect(track, &Track::IndexChanged, this, &TimelineWidget::TrackUpdated);
  disconnect(track, &Track::IndexChanged, this, &TimelineWidget::TrackIndexChanged);
  disconnect(track, &Track::BlocksRefreshed, this, &TimelineWidget::TrackUpdated);
  disconnect(track, &Track::TrackHeightChanged, this, &TimelineWidget::TrackUpdated);
  disconnect(track, &Track::BlockAdded, this, &TimelineWidget::AddBlock);
  disconnect(track, &Track::BlockRemoved, this, &TimelineWidget::RemoveBlock);

  RemoveSelection(TimeRange(0, RATIONAL_MAX), track->ToReference());

  foreach (Block* b, track->Blocks()) {
    RemoveBlock(b);
  }
}

void TimelineWidget::TrackUpdated()
{
  UpdateViewports(static_cast<Track*>(sender())->type());
}

void TimelineWidget::BlockUpdated()
{
  UpdateViewports(static_cast<Block*>(sender())->track()->type());
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

void TimelineWidget::ShowContextMenu()
{
  Menu menu(this);

  QVector<Block*> selected = GetSelectedBlocks();

  if (!selected.isEmpty()) {
    MenuShared::instance()->AddItemsForEditMenu(&menu, true);

    menu.addSeparator();

    MenuShared::instance()->AddColorCodingMenu(&menu);

    menu.addSeparator();

    if (ClipBlock *clip = dynamic_cast<ClipBlock*>(selected.first())) {
      {
        Menu *cache_menu = new Menu(tr("Cache"), &menu);
        menu.addMenu(cache_menu);

        QAction *autocache_action = cache_menu->addAction(tr("Auto-Cache"));
        autocache_action->setCheckable(true);
        autocache_action->setChecked(clip->IsAutocaching());
        connect(autocache_action, &QAction::triggered, this, &TimelineWidget::SetSelectedClipsAutocaching);

        cache_menu->addSeparator();

        auto cache_clip = cache_menu->addAction(tr("Cache All"));
        connect(cache_clip, &QAction::triggered, this, &TimelineWidget::CacheClips);

        auto cache_inout = cache_menu->addAction(tr("Cache In/Out"));
        connect(cache_inout, &QAction::triggered, this, &TimelineWidget::CacheClipsInOut);

        auto cache_discard = cache_menu->addAction(tr("Discard"));
        connect(cache_discard, &QAction::triggered, this, &TimelineWidget::CacheDiscard);
      }

      if (clip->connected_viewer()) {
        QAction *reveal_in_footage_viewer = menu.addAction(tr("Reveal in Footage Viewer"));
        reveal_in_footage_viewer->setData(reinterpret_cast<quintptr>(clip->connected_viewer()));
        reveal_in_footage_viewer->setProperty("range", QVariant::fromValue(clip->media_range()));
        connect(reveal_in_footage_viewer, &QAction::triggered, this, &TimelineWidget::RevealInFootageViewer);

        QAction *reveal_in_project = menu.addAction(tr("Reveal in Project"));
        reveal_in_project->setData(reinterpret_cast<quintptr>(clip->connected_viewer()));
        connect(reveal_in_project, &QAction::triggered, this, &TimelineWidget::RevealInProject);

        if (Sequence *sequence = dynamic_cast<Sequence*>(clip->connected_viewer())) {
          QAction *multicam_enabled = menu.addAction(tr("Multi-Cam"));
          multicam_enabled->setCheckable(true);

          MultiCamNode *mcn = nullptr;
          auto paths = clip->FindWaysNodeArrivesHere(sequence);

          for (const NodeInput &i : paths) {
            if ((mcn = dynamic_cast<MultiCamNode*>(i.node()))) {
              break;
            }
          }

          multicam_enabled->setChecked(mcn);

          connect(multicam_enabled, &QAction::triggered, this, &TimelineWidget::MulticamEnabledTriggered);
        }
      }
    }

    menu.addSeparator();

    QAction* properties_action = menu.addAction(tr("Properties"));
    connect(properties_action, &QAction::triggered, this, &TimelineWidget::ShowSpeedDurationDialogForSelectedClips);
  }

  if (selected.isEmpty()) {

    QAction* toggle_audio_units = menu.addAction(tr("Use Audio Time Units"));
    toggle_audio_units->setCheckable(true);
    toggle_audio_units->setChecked(use_audio_time_units_);
    connect(toggle_audio_units, &QAction::triggered, this, &TimelineWidget::SetUseAudioTimeUnits);

    {
      Menu *thumbnail_menu = new Menu(tr("Show Thumbnails"), &menu);
      menu.addMenu(thumbnail_menu);

      thumbnail_menu->AddActionWithData(tr("Disabled"), Timeline::kThumbnailOff, OLIVE_CONFIG("TimelineThumbnailMode"));
      thumbnail_menu->AddActionWithData(tr("Only At In Points"), Timeline::kThumbnailInOut, OLIVE_CONFIG("TimelineThumbnailMode"));
      thumbnail_menu->AddActionWithData(tr("Enabled"), Timeline::kThumbnailOn, OLIVE_CONFIG("TimelineThumbnailMode"));

      connect(thumbnail_menu, &Menu::triggered, this, &TimelineWidget::SetViewThumbnailsEnabled);
    }

    QAction* show_waveforms = menu.addAction(tr("Show Waveforms"));
    show_waveforms->setCheckable(true);
    show_waveforms->setChecked(OLIVE_CONFIG("TimelineWaveformMode").toInt() == Timeline::kWaveformsEnabled);
    connect(show_waveforms, &QAction::triggered, this, &TimelineWidget::SetViewWaveformsEnabled);

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

  SequenceDialog sd(sequence(), SequenceDialog::kExisting, this);
  sd.exec();
}

void TimelineWidget::SetUseAudioTimeUnits(bool use)
{
  use_audio_time_units_ = use;

  // Update timebases
  UpdateViewTimebases();
}

void TimelineWidget::ToolChanged()
{
  HideSnaps();
  SetViewBeamCursor(TimelineCoordinate(0, Track::kNone, -1));
  SetViewTransitionOverlay(nullptr, nullptr);

  AddableObjectChanged();
}

void TimelineWidget::AddableObjectChanged()
{
  // Special cast for subtitle adding - ensure section is visible
  if (Core::instance()->tool() == Tool::kAdd && Core::instance()->GetSelectedAddableObject() == Tool::kAddableSubtitle) {
    AddTentativeSubtitleTrack();
  } else {
    ClearTentativeSubtitleTrack();
  }
}

void TimelineWidget::SetViewWaveformsEnabled(bool e)
{
  OLIVE_CONFIG("TimelineWaveformMode") = e ? Timeline::kWaveformsEnabled : Timeline::kWaveformsDisabled;
  UpdateViewports();
}

void TimelineWidget::SetViewThumbnailsEnabled(QAction *action)
{
  OLIVE_CONFIG("TimelineThumbnailMode") = action->data();
  UpdateViewports();
}

void TimelineWidget::FrameRateChanged()
{
  SetTimebase(GetConnectedNode()->GetVideoParams().frame_rate_as_time_base());
}

void TimelineWidget::SampleRateChanged()
{
  UpdateViewTimebases();
}

void TimelineWidget::TrackIndexChanged(int old, int now)
{
  Track* track = static_cast<Track*>(sender());

  Track::Reference old_ref(track->type(), old);
  Track::Reference new_ref(track->type(), now);

  auto track_selections = selections_.take(old_ref);
  if (!track_selections.isEmpty()) {
    selections_.insert(new_ref, track_selections);
  }
}

void TimelineWidget::SignalBlockSelectionChange()
{
  signal_block_change_timer_->stop();
  signal_block_change_timer_->start();
}

void TimelineWidget::RevealInFootageViewer()
{
  QAction *a = static_cast<QAction*>(sender());

  ViewerOutput *item_to_reveal = reinterpret_cast<ViewerOutput*>(a->data().value<quintptr>());
  TimeRange r = a->property("range").value<TimeRange>();

  emit RevealViewerInFootageViewer(item_to_reveal, r);
}

void TimelineWidget::RevealInProject()
{
  QAction *a = static_cast<QAction*>(sender());

  ViewerOutput *item_to_reveal = reinterpret_cast<ViewerOutput*>(a->data().value<quintptr>());

  emit RevealViewerInProject(item_to_reveal);
}

void TimelineWidget::RenameSelectedBlocks()
{
  MultiUndoCommand *command = new MultiUndoCommand();
  QVector<Node*> nodes(selected_blocks_.size());

  for (int i=0; i<nodes.size(); i++) {
    nodes[i] = selected_blocks_[i];
  }

  Core::instance()->LabelNodes(nodes);
  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void TimelineWidget::TrackAboutToBeDeleted(Track *track)
{
  if (track == subtitle_tentative_track_) {
    // User is deleting the tentative subtitle track. Technically they shouldn't do this, but they
    // might if they misinterpret it as permanent. If so, we handle it cleanly by pushing our
    // command as if the action really were permanent.
    Core::instance()->undo_stack()->push(TakeSubtitleSectionCommand());
  }
}

void TimelineWidget::SetSelectedClipsAutocaching(bool e)
{
  MultiUndoCommand *command = new MultiUndoCommand();

  for (Block *b : selected_blocks_) {
    if (ClipBlock *clip = dynamic_cast<ClipBlock*>(b)) {
      command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(NodeInput(clip, ClipBlock::kAutoCacheInput)), e));
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void TimelineWidget::CacheClips()
{
  for (Block *b : selected_blocks_) {
    if (ClipBlock *clip = dynamic_cast<ClipBlock*>(b)) {
      clip->RequestInvalidatedFromConnected(true);
    }
  }
}

void TimelineWidget::CacheClipsInOut()
{
  if (!this->sequence() || !this->sequence()->GetWorkArea()->enabled()) {
    return;
  }

  TimeTargetObject tto;
  tto.SetTimeTarget(this->sequence());

  const TimeRange &r = this->sequence()->GetWorkArea()->range();
  for (Block *b : qAsConst(selected_blocks_)) {
    if (ClipBlock *clip = dynamic_cast<ClipBlock*>(b)) {
      if (Node *connected = clip->GetConnectedOutput(clip->kBufferIn)) {
        TimeRange adjusted = tto.GetAdjustedTime(this->sequence(), connected, r, Node::kTransformTowardsInput);
        clip->RequestInvalidatedFromConnected(true, adjusted);
      }
    }
  }
}

void TimelineWidget::CacheDiscard()
{
  if (QMessageBox::question(this, tr("Discard Cache"),
                            tr("This will discard all cache for this clip. "
                               "If the clip has auto-cache enabled, it will be recached immediately. "
                               "This cannot be undone.\n\n"
                               "Do you wish to continue?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
    for (Block *b : selected_blocks_) {
      if (ClipBlock *clip = dynamic_cast<ClipBlock*>(b)) {
        clip->DiscardCache();
      }
    }
  }
}

void TimelineWidget::MulticamEnabledTriggered(bool e)
{
  MultiUndoCommand *command = new MultiUndoCommand();

  for (Block *b : qAsConst(selected_blocks_)) {
    if (ClipBlock *c = dynamic_cast<ClipBlock*>(b)) {
      if (Sequence *s = dynamic_cast<Sequence*>(c->connected_viewer())) {
        if (e) {

          // Adding multicams
          // Create multicam node and add it to the graph
          MultiCamNode *n = new MultiCamNode();
          n->SetSequenceType(c->GetTrackType());
          command->add_child(new NodeAddCommand(s->parent(), n));


          // For each output the sequence has to this clip, disconnect it and
          // connect to the multicam instead
          QVector<NodeInput> inputs = c->FindWaysNodeArrivesHere(s);
          for (const NodeInput &i : inputs) {
            command->add_child(new NodeEdgeRemoveCommand(s, i));
            command->add_child(new NodeEdgeAddCommand(n, i));
          }

          command->add_child(new NodeEdgeAddCommand(s, NodeInput(n, n->kSequenceInput)));

          // Move sequence node one unit back, and place multicam in sequence's spot
          QPointF sequence_pos = c->GetNodePositionInContext(s);
          command->add_child(new NodeSetPositionCommand(s, c, sequence_pos - QPointF(1, 0)));
          command->add_child(new NodeSetPositionCommand(n, c, sequence_pos));

        } else {

          // Removing multicams
          // Locate first multicam that specifically ends up at this clip
          QVector<NodeInput> inputs = c->FindWaysNodeArrivesHere(s);
          for (const NodeInput &i : inputs) {
            if (MultiCamNode *mcn = dynamic_cast<MultiCamNode*>(i.node())) {
              for (auto it=mcn->output_connections().cbegin(); it!=mcn->output_connections().cend(); it++) {
                command->add_child(new NodeEdgeRemoveCommand(it->first, it->second));
                command->add_child(new NodeEdgeAddCommand(s, it->second));
              }

              command->add_child(new NodeRemoveAndDisconnectCommand(mcn));
            }
          }

        }
      }
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void TimelineWidget::ForceUpdateRubberBand()
{
  if (rubberband_.isVisible()) {
    this->MoveRubberBandSelect(rubberband_enable_selecting_, rubberband_select_links_);
  }
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

    if (GetConnectedNode() && use_audio_time_units_ && i == Track::kAudio) {
      view->view()->SetTimebase(GetConnectedNode()->GetAudioParams().sample_rate_as_time_base());
    } else {
      view->view()->SetTimebase(timebase());
    }
  }
}

void TimelineWidget::NudgeInternal(rational amount)
{
  if (!selected_blocks_.isEmpty()) {
    // Validate
    foreach (Block* b, selected_blocks_) {
      if (b->in() + amount < 0) {
        amount = -b->in();
      }
    }

    if (amount.isNull()) {
      return;
    }

    MultiUndoCommand *command = new MultiUndoCommand();

    foreach (Block* b, selected_blocks_) {
      command->add_child(new TrackReplaceBlockWithGapCommand(b->track(), b, false));
    }

    foreach (Block* b, selected_blocks_) {
      command->add_child(new TrackPlaceBlockCommand(sequence()->track_list(b->track()->type()), b->track()->Index(), b, b->in() + amount));
    }

    // Nudge selections
    TimelineWidgetSelections new_sel = GetSelections();
    new_sel.ShiftTime(amount);
    command->add_child(new TimelineWidget::SetSelectionsCommand(this, new_sel, GetSelections()));

    Core::instance()->undo_stack()->push(command);
  }
}

void TimelineWidget::MoveToPlayheadInternal(bool out)
{
  if (GetConnectedNode() && !selected_blocks_.isEmpty()) {
    MultiUndoCommand *command = new MultiUndoCommand();

    // Remove each block from the graph
    QHash<Track*, rational> earliest_pts;
    foreach (Block *b, selected_blocks_) {
      command->add_child(new TrackReplaceBlockWithGapCommand(b->track(), b, false));

      rational r = earliest_pts.value(b->track(), out ? RATIONAL_MIN : RATIONAL_MAX);
      rational compare = out ? b->out() : b->in();
      if ((compare < r) == !out) {
        earliest_pts.insert(b->track(), compare);
      }
    }

    foreach (Block *b, selected_blocks_) {
      rational shift_amt = GetConnectedNode()->GetPlayhead() - earliest_pts.value(b->track());
      rational new_in = b->in() + shift_amt;
      bool can_shift = true;

      if (new_in < 0) {
        // Handle clips threatening to go below 0
        rational new_out = new_in + b->length();
        if (new_out <= 0) {
          can_shift = false;
        } else {
          command->add_child(new BlockResizeWithMediaInCommand(b, new_out));
          new_in = 0;
        }
      }

      if (can_shift) {
        command->add_child(new TrackPlaceBlockCommand(sequence()->track_list(b->track()->type()), b->track()->Index(), b, new_in));
      }
    }

    // Shift selections
    TimelineWidgetSelections new_sel = GetSelections();
    for (auto it=new_sel.begin(); it!=new_sel.end(); it++) {
      rational track_adj = GetConnectedNode()->GetPlayhead() - earliest_pts.value(GetTrackFromReference(it.key()), GetConnectedNode()->GetPlayhead());
      if (!track_adj.isNull()) {
        it.value().shift(track_adj);
      }
    }
    command->add_child(new SetSelectionsCommand(this, new_sel, GetSelections()));

    Core::instance()->undo_stack()->push(command);
  }
}

void TimelineWidget::SetViewBeamCursor(const TimelineCoordinate &coord)
{
  foreach (TimelineAndTrackView* tview, views_) {
    tview->view()->SetBeamCursor(coord);
  }
}

void TimelineWidget::SetViewTransitionOverlay(ClipBlock *out, ClipBlock *in)
{
  foreach (TimelineAndTrackView* tview, views_) {
    tview->view()->SetTransitionOverlay(out, in);
  }
}

void TimelineWidget::SetBlockLinksSelected(ClipBlock* block, bool selected)
{
  foreach (Block* link, block->block_links()) {
    if (selected) {
      AddSelection(link);
    } else {
      RemoveSelection(link);
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

rational TimelineWidget::GetTimebaseForTrackType(Track::Type type)
{
  return views_.at(type)->view()->timebase();
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

  SignalBlockSelectionChange();
}

void TimelineWidget::SignalDeselectedBlocks(const QVector<Block *> &deselected_blocks)
{
  if (deselected_blocks.isEmpty()) {
    return;
  }

  foreach (Block* b, deselected_blocks) {
    selected_blocks_.removeOne(b);
  }

  SignalBlockSelectionChange();
}

void TimelineWidget::SignalDeselectedAllBlocks()
{
  if (!selected_blocks_.isEmpty()) {
    selected_blocks_.clear();
    SignalBlockSelectionChange();
  }
}

QVector<Timeline::EditToInfo> TimelineWidget::GetEditToInfo(const rational& playhead_time,
                                                            Timeline::MovementMode mode)
{
  // Get list of unlocked tracks
  QVector<Track*> tracks = sequence()->GetUnlockedTracks();

  // Create list to cache nearest times and the blocks at this point
  QVector<Timeline::EditToInfo> info_list(tracks.size());

  for (int i=0;i<tracks.size();i++) {
    Timeline::EditToInfo &info = info_list[i];

    Track* track = tracks.at(i);
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
  }

  return info_list;
}

void TimelineWidget::RippleTo(Timeline::MovementMode mode)
{
  if (!GetConnectedNode()) {
    return;
  }

  rational playhead_time = GetConnectedNode()->GetPlayhead();

  QVector<Timeline::EditToInfo> tracks = GetEditToInfo(playhead_time, mode);

  if (tracks.isEmpty()) {
    return;
  }

  // Find each track's nearest point and determine the overall timeline's nearest point
  rational closest_point_to_playhead = (mode == Timeline::kTrimIn) ? RATIONAL_MIN : RATIONAL_MAX;

  foreach (const Timeline::EditToInfo& info, tracks) {
    if (info.nearest_block) {
      if (mode == Timeline::kTrimIn) {
        closest_point_to_playhead = qMax(info.nearest_time, closest_point_to_playhead);
      } else {
        closest_point_to_playhead = qMin(info.nearest_time, closest_point_to_playhead);
      }
    }
  }

  if (closest_point_to_playhead == RATIONAL_MIN || closest_point_to_playhead == RATIONAL_MAX) {
    // Assume no blocks will be acted upon
    return;
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

  TimelineRippleRemoveAreaCommand* c = new TimelineRippleRemoveAreaCommand(sequence(),
                                                                           in_ripple,
                                                                           out_ripple);

  Core::instance()->undo_stack()->push(c);

  // If we rippled, ump to where new cut is if applicable
  if (mode == Timeline::kTrimIn) {
    GetConnectedNode()->SetPlayhead(closest_point_to_playhead);
  } else if (mode == Timeline::kTrimOut && closest_point_to_playhead == GetConnectedNode()->GetPlayhead()) {
    GetConnectedNode()->SetPlayhead(playhead_time);
  }
}

void TimelineWidget::EditTo(Timeline::MovementMode mode)
{
  const rational playhead_time = GetConnectedNode()->GetPlayhead();

  // Get list of unlocked tracks
  QVector<Timeline::EditToInfo> tracks = GetEditToInfo(playhead_time, mode);

  if (tracks.isEmpty()) {
    return;
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  foreach (const Timeline::EditToInfo& info, tracks) {
    if (info.nearest_block
        && !dynamic_cast<GapBlock*>(info.nearest_block)
        && info.nearest_time != playhead_time) {
      rational new_len;

      if (mode == Timeline::kTrimIn) {
        new_len = playhead_time - info.nearest_time;
      } else {
        new_len = info.nearest_time - playhead_time;
      }
      new_len = info.nearest_block->length() - new_len;

      command->add_child(new BlockTrimCommand(info.track,
                                              info.nearest_block,
                                              new_len,
                                              mode));
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void TimelineWidget::UpdateViewports(const Track::Type &type)
{
  if (type == Track::kNone) {
    foreach (TimelineAndTrackView* tview, views_) {
      tview->view()->viewport()->update();
    }
  } else {
    views_.at(type)->view()->viewport()->update();
  }
}

bool TimelineWidget::PasteInternal(bool insert)
{
  if (!GetConnectedNode()) {
    return false;
  }

  ProjectSerializer::Result res = ProjectSerializer::Paste(QStringLiteral("timeline"));
  if (res.GetLoadedNodes().isEmpty()) {
    return false;
  }

  MultiUndoCommand *command = new MultiUndoCommand();

  foreach (Node *n, res.GetLoadedNodes()) {
    command->add_child(new NodeAddCommand(GetConnectedNode()->project(), n));
  }

  rational paste_start = GetConnectedNode()->GetPlayhead();

  if (insert) {
    rational paste_end = paste_start;

    for (auto it=res.GetLoadData().properties.cbegin(); it!=res.GetLoadData().properties.cend(); it++) {
      rational length = static_cast<Block*>(it.key())->length();
      rational in = rational::fromString(it.value()[QStringLiteral("in")].toStdString());

      paste_end = qMax(paste_end, paste_start + in + length);
    }

    if (paste_end != paste_start) {
      InsertGapsAt(paste_start, paste_end - paste_start, command);
    }
  }

  for (auto it=res.GetLoadData().properties.cbegin(); it!=res.GetLoadData().properties.cend(); it++) {
    Block *block = static_cast<Block*>(it.key());
    rational in = rational::fromString(it.value()[QStringLiteral("in")].toStdString());
    Track::Reference track = Track::Reference::FromString(it.value()[QStringLiteral("track")]);

    command->add_child(new TrackPlaceBlockCommand(sequence()->track_list(track.type()),
                                                  track.index(),
                                                  block,
                                                  paste_start + in));
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  return true;
}

TimelineAndTrackView *TimelineWidget::AddTimelineAndTrackView(Qt::Alignment alignment)
{
  TimelineAndTrackView *v = new TimelineAndTrackView(alignment);
  connect(v->track_view(), &TrackView::AboutToDeleteTrack, this, &TimelineWidget::TrackAboutToBeDeleted);
  return v;
}

QHash<Node *, Node *> TimelineWidget::GenerateExistingPasteMap(const ProjectSerializer::Result &r)
{
  QHash<Node *, Node *> m;

  for (Node *n : r.GetLoadedNodes()) {
    for (Block *b : qAsConst(this->selected_blocks_)) {
      for (auto it=b->GetContextPositions().cbegin(); it!=b->GetContextPositions().cend(); it++) {
        if (it.key()->id() == n->id() && !m.contains(it.key())) {
          m.insert(it.key(), n);
          break;
        }
      }
    }
  }

  return m;
}

QByteArray TimelineWidget::SaveSplitterState() const
{
  return view_splitter_->saveState();
}

void TimelineWidget::RestoreSplitterState(const QByteArray &state)
{
  view_splitter_->restoreState(state);
}

void TimelineWidget::StartRubberBandSelect(const QPoint &global_cursor_start)
{
  // Store scene positions for each view
  rubberband_scene_pos_.resize(views_.size());
  for (int i = 0; i < rubberband_scene_pos_.size(); i++) {
    TimelineView *v = views_.at(i)->view();
    rubberband_scene_pos_[i] = v->UnscalePoint(v->mapToScene(v->mapFromGlobal(global_cursor_start)));
  }

  rubberband_.show();

  // We don't touch any blocks that are already selected. If you want these to be deselected by
  // default, call DeselectAll() before calling StartRubberBandSelect()
  rubberband_old_selections_ = selections_;
}

void TimelineWidget::MoveRubberBandSelect(bool enable_selecting, bool select_links)
{
  QPoint rubberband_now = QCursor::pos();

  TimelineView *fv = views_.first()->view();
  const QPointF &rubberband_scene_start = rubberband_scene_pos_.at(0);
  QPointF rubberband_now_scaled = fv->UnscalePoint(fv->mapToScene(fv->mapFromGlobal(rubberband_now)));

  QPoint rubberband_local_start = fv->mapTo(this, fv->mapFromScene(fv->ScalePoint(rubberband_scene_start)));
  QPoint rubberband_local_now = fv->mapTo(this, fv->mapFromScene(fv->ScalePoint(rubberband_now_scaled)));

  rubberband_.setGeometry(QRect(rubberband_local_start, rubberband_local_now).normalized());

  rubberband_enable_selecting_ = enable_selecting;
  rubberband_select_links_ = select_links;

  if (!enable_selecting) {
    return;
  }

  // Get current items in rubberband
  QVector<Block*> items_in_rubberband;

  for (int i = 0; i < views_.size(); i++) {
    TimelineView *v = views_.at(i)->view();
    QRectF r = QRectF(v->ScalePoint(rubberband_scene_pos_.at(i)), v->mapToScene(v->mapFromGlobal(rubberband_now))).normalized();
    items_in_rubberband.append(v->GetItemsAtSceneRect(r));
  }

  // Reset selection to whatever it was before
  SetSelections(rubberband_old_selections_, false);

  // Add any blocks in rubberband
  rubberband_now_selected_.clear();

  foreach (Block* b, items_in_rubberband) {
    if (dynamic_cast<GapBlock*>(b)) {
      continue;
    }

    Track* t = b->track();
    if (t->IsLocked()) {
      continue;
    }

    if (!rubberband_now_selected_.contains(b)) {
      AddSelection(b);
      rubberband_now_selected_.append(b);
    }

    ClipBlock *c = dynamic_cast<ClipBlock*>(b);
    if (c && select_links) {
      foreach (Block* link, c->block_links()) {
        if (!rubberband_now_selected_.contains(link)) {
          AddSelection(link);
          rubberband_now_selected_.append(link);
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

void TimelineWidget::AddSelection(const TimeRange &time, const Track::Reference &track)
{
  selections_[track].insert(time);

  UpdateViewports(track.type());
}

void TimelineWidget::AddSelection(Block *item)
{
  if (item->track()) {
    AddSelection(item->range(), item->track()->ToReference());
  }
}

void TimelineWidget::RemoveSelection(const TimeRange &time, const Track::Reference &track)
{
  selections_[track].remove(time);

  UpdateViewports(track.type());
}

void TimelineWidget::RemoveSelection(Block *item)
{
  if (item->track()) {
    RemoveSelection(item->range(), item->track()->ToReference());
  }
}

void TimelineWidget::SetSelections(const TimelineWidgetSelections &s, bool process_block_changes)
{
  if (selections_ == s) {
    return;
  }

  if (!GetConnectedNode()) {
    return;
  }

  if (process_block_changes) {
    QVector<Block*> deselected;
    QVector<Block*> selected;

    foreach (Block *b, selected_blocks_) {
      if (!s[b->track()->ToReference()].contains(b->range())) {
        deselected.append(b);
      }
    }

    // NOTE: This loop could do with some optimization
    for (auto it=s.cbegin(); it!=s.cend(); it++) {
      Track *track = GetTrackFromReference(it.key());
      if (track) {
        const TimeRangeList &ranges = it.value();

        foreach (Block *b, track->Blocks()) {
          if (!selected_blocks_.contains(b) && ranges.contains(b->range())) {
            selected.append(b);
          }
        }
      }
    }

    SignalDeselectedBlocks(deselected);
    SignalSelectedBlocks(selected);
  }

  selections_ = s;

  UpdateViewports();
}

Block *TimelineWidget::GetItemAtScenePos(const TimelineCoordinate& coord)
{
  return views_.at(coord.GetTrack().type())->view()->GetItemAtScenePos(coord.GetFrame(), coord.GetTrack().index());
}

void TimelineWidget::SetSplitterSizesCommand::redo()
{
  old_sizes_ = splitter_->sizes();
  splitter_->setSizes(new_sizes_);
}

void TimelineWidget::SetSplitterSizesCommand::undo()
{
  splitter_->setSizes(old_sizes_);
}

}
