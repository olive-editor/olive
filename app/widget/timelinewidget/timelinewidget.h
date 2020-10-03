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

#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QScrollBar>
#include <QRubberBand>
#include <QWidget>

#include "core.h"
#include "node/block/transition/transition.h"
#include "node/output/viewer/viewer.h"
#include "snapservice.h"
#include "timeline/timelinecommon.h"
#include "timelineandtrackview.h"
#include "widget/nodecopypaste/nodecopypaste.h"
#include "widget/slider/timeslider.h"
#include "widget/timebased/timebased.h"
#include "widget/timelinewidget/timelinewidgetselections.h"
#include "widget/timelinewidget/tool/import.h"
#include "widget/timelinewidget/tool/tool.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief Full widget for working with TimelineOutput nodes
 *
 * Encapsulates TimelineViews, TimeRulers, and scrollbars for a complete widget to manipulate Timelines
 */
class TimelineWidget : public TimeBasedWidget, public NodeCopyPasteWidget, public SnapService
{
  Q_OBJECT
public:
  TimelineWidget(QWidget* parent = nullptr);

  virtual ~TimelineWidget() override;

  void Clear();

  void SelectAll();

  void DeselectAll();

  void RippleToIn();

  void RippleToOut();

  void EditToIn();

  void EditToOut();

  void SplitAtPlayhead();

  void DeleteSelected(bool ripple = false);

  void IncreaseTrackHeight();

  void DecreaseTrackHeight();

  void InsertFootageAtPlayhead(const QList<Footage *> &footage);

  void OverwriteFootageAtPlayhead(const QList<Footage *> &footage);

  void ToggleLinksOnSelected();

  void CopySelected(bool cut);

  void Paste(bool insert);

  void DeleteInToOut(bool ripple);

  void ToggleSelectedEnabled();

  QList<TimelineViewBlockItem*> GetSelectedBlocks();

  virtual bool SnapPoint(QList<rational> start_times, rational *movement, int snap_points = kSnapAll) override;

  virtual void HideSnaps() override;

  QByteArray SaveSplitterState() const;

  void RestoreSplitterState(const QByteArray& state);

  static void ReplaceBlocksWithGaps(const QList<Block *>& blocks, bool remove_from_graph, QUndoCommand* command);

  /**
   * @brief Retrieve the QGraphicsItem at a particular scene position
   *
   * Requires a float-based scene position. If you have a screen position, use GetScenePos() first to convert it to a
   * scene position
   */
  TimelineViewBlockItem* GetItemAtScenePos(const TimelineCoordinate &coord);

  const QMap<Block*, TimelineViewBlockItem*>& GetBlockItems() const
  {
    return block_items_;
  }

  void AddSelection(const TimeRange& time, const TrackReference& track);
  void AddSelection(TimelineViewBlockItem* item);

  void RemoveSelection(const TimeRange& time, const TrackReference& track);
  void RemoveSelection(TimelineViewBlockItem* item);

  const TimelineWidgetSelections& GetSelections() const
  {
    return selections_;
  }

  void SetSelections(const TimelineWidgetSelections &s);

  TrackOutput* GetTrackFromReference(const TrackReference& ref);

  void SetViewBeamCursor(const TimelineCoordinate& coord);

  const QVector<TimelineViewGhostItem*>& GetGhostItems() const
  {
    return ghost_items_;
  }

  void InsertGapsAt(const rational& time, const rational& length, QUndoCommand* command);

  void StartRubberBandSelect(bool enable_selecting, bool select_links);
  void MoveRubberBandSelect(bool enable_selecting, bool select_links);
  void EndRubberBandSelect();

  int GetTrackY(const TrackReference& ref);
  int GetTrackHeight(const TrackReference& ref);

  void AddGhost(TimelineViewGhostItem* ghost);

  void ClearGhosts();

  bool HasGhosts() const
  {
    return !ghost_items_.isEmpty();
  }

  rational GetToolTipTimebase() const;

  bool IsBlockSelected(Block* b) const
  {
    return selected_blocks_.contains(b);
  }

  void SetBlockLinksSelected(Block *block, bool selected);

  void QueueScroll(int value);

  TimelineView* GetFirstTimelineView();

  const QRect &GetRubberBandGeometry() const;

  void SignalSelectedBlocks(const QList<Block*>& selected_blocks);

  void SignalDeselectedBlocks(const QList<Block*>& deselected_blocks);

signals:
  void BlocksSelected(const QList<Block*>& selected_blocks);

  void BlocksDeselected(const QList<Block*>& deselected_blocks);

protected:
  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void TimebaseChangedEvent(const rational &) override;
  virtual void TimeChangedEvent(const int64_t &) override;
  virtual void ScaleChangedEvent(const double &) override;

  virtual void ConnectNodeInternal(ViewerOutput* n) override;
  virtual void DisconnectNodeInternal(ViewerOutput* n) override;

  virtual void CopyNodesToClipboardInternal(QXmlStreamWriter *writer, void* userdata) override;
  virtual void PasteNodesFromClipboardInternal(QXmlStreamReader *reader, void* userdata) override;

  struct BlockPasteData {
    quintptr ptr;
    rational in;
    Timeline::TrackType track_type;
    int track_index;
  };

private:
  QVector<Timeline::EditToInfo> GetEditToInfo(const rational &playhead_time, Timeline::MovementMode mode);

  void RippleTo(Timeline::MovementMode mode);

  void EditTo(Timeline::MovementMode mode);

  void ShowSnap(const QList<rational>& times);

  void UpdateViewports(const Timeline::TrackType& type = Timeline::kTrackTypeNone);

  QPoint drag_origin_;

  QRubberBand rubberband_;
  QList<QGraphicsItem*> rubberband_already_selected_;
  QList<QGraphicsItem*> rubberband_now_selected_;

  TimelineWidgetSelections selections_;

  TimelineTool* GetActiveTool();

  QVector<TimelineTool*> tools_;

  ImportTool* import_tool_;

  TimelineTool* active_tool_;

  QVector<TimelineViewGhostItem*> ghost_items_;

  QMap<Block*, TimelineViewBlockItem*> block_items_;

  QList<TimelineAndTrackView*> views_;

  TimeSlider* timecode_label_;

  QList<Block*> selected_blocks_;

  int deferred_scroll_value_;

  bool use_audio_time_units_;

  QSplitter* view_splitter_;

  void CenterOn(qreal scene_pos);

  void UpdateViewTimebases();

private slots:
  void ViewMousePressed(TimelineViewMouseEvent* event);
  void ViewMouseMoved(TimelineViewMouseEvent* event);
  void ViewMouseReleased(TimelineViewMouseEvent* event);
  void ViewMouseDoubleClicked(TimelineViewMouseEvent* event);

  void ViewDragEntered(TimelineViewMouseEvent* event);
  void ViewDragMoved(TimelineViewMouseEvent* event);
  void ViewDragLeft(QDragLeaveEvent* event);
  void ViewDragDropped(TimelineViewMouseEvent* event);

  void AddBlock(Block* block, TrackReference track);
  void RemoveBlock(const QList<Block*>& blocks);

  void AddTrack(TrackOutput* track, Timeline::TrackType type);
  void RemoveTrack(TrackOutput* track);
  void TrackIndexChanged();

  /**
   * @brief Slot for when a Block node changes its parameters and the graphics need to update
   *
   * This slot does a static_cast on sender() to Block*, meaning all objects triggering this slot must be Blocks or
   * derivatives.
   */
  void BlockRefreshed();

  void BlockUpdated();

  void TrackPreviewUpdated();

  void UpdateHorizontalSplitters();

  void UpdateTimecodeWidthFromSplitters(QSplitter *s);

  void TrackHeightChanged(Timeline::TrackType type, int index, int height);

  void ShowContextMenu();

  void DeferredScrollAction();

  void ShowSequenceDialog();

  void SetUseAudioTimeUnits(bool use);

  void SetViewTimestamp(const int64_t& ts);

  void ViewTimestampChanged(int64_t ts);

};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINEWIDGET_H
