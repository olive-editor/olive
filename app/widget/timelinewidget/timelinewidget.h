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
  enum DropWithoutSequenceBehavior {
    kDWSAsk,
    kDWSAuto,
    kDWSManual,
    kDWSDisable
  };

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

signals:
  void SelectionChanged(const QList<Block*>& selected_blocks);

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
  class DraggedFootage {
  public:
    DraggedFootage(Footage* f, quint64 streams) :
      footage_(f),
      streams_(streams)
    {
    }

    Footage* footage() const {
      return footage_;
    }

    const quint64& streams() const {
      return streams_;
    }

  private:
    Footage* footage_;

    quint64 streams_;

  };

  static DraggedFootage FootageToDraggedFootage(Footage* f);
  static QList<DraggedFootage> FootageToDraggedFootage(QList<Footage*> footage);

  class Tool
  {
  public:
    Tool(TimelineWidget* parent);
    virtual ~Tool();

    virtual void MousePress(TimelineViewMouseEvent *){}
    virtual void MouseMove(TimelineViewMouseEvent *){}
    virtual void MouseRelease(TimelineViewMouseEvent *){}
    virtual void MouseDoubleClick(TimelineViewMouseEvent *){}

    virtual void HoverMove(TimelineViewMouseEvent *){}

    virtual void DragEnter(TimelineViewMouseEvent *){}
    virtual void DragMove(TimelineViewMouseEvent *){}
    virtual void DragLeave(QDragLeaveEvent *){}
    virtual void DragDrop(TimelineViewMouseEvent *){}

    TimelineWidget* parent();

    static Timeline::MovementMode FlipTrimMode(const Timeline::MovementMode& trim_mode);

  protected:
    /**
     * @brief Retrieve the QGraphicsItem at a particular scene position
     *
     * Requires a float-based scene position. If you have a screen position, use GetScenePos() first to convert it to a
     * scene position
     */
    TimelineViewBlockItem* GetItemAtScenePos(const TimelineCoordinate &coord);

    /**
     * @brief Validates Ghosts that are moving horizontally (time-based)
     *
     * Validation is the process of ensuring that whatever movements the user is making are "valid" and "legal". This
     * function's validation ensures that no Ghost's in point ends up in a negative timecode.
     */
    rational ValidateTimeMovement(rational movement);

    /**
     * @brief Validates Ghosts that are moving vertically (track-based)
     *
     * This function's validation ensures that no Ghost's track ends up in a negative (non-existent) track.
     */
    int ValidateTrackMovement(int movement, const QVector<TimelineViewGhostItem *> &ghosts);

    void GetGhostData(rational *earliest_point, rational *latest_point);

    void InsertGapsAtGhostDestination(QUndoCommand* command);

    QList<rational> snap_points_;

    bool dragging_;

    TimelineCoordinate drag_start_;

  private:
    TimelineWidget* parent_;

  };

  class BeamTool : public Tool
  {
  public:
    BeamTool(TimelineWidget *parent);

    virtual void HoverMove(TimelineViewMouseEvent *event) override;

  protected:
    TimelineCoordinate ValidatedCoordinate(TimelineCoordinate coord);

  };

  class PointerTool : public Tool
  {
  public:
    PointerTool(TimelineWidget* parent);

    virtual void MousePress(TimelineViewMouseEvent *event) override;
    virtual void MouseMove(TimelineViewMouseEvent *event) override;
    virtual void MouseRelease(TimelineViewMouseEvent *event) override;

    virtual void HoverMove(TimelineViewMouseEvent *event) override;

  protected:
    virtual void FinishDrag(TimelineViewMouseEvent *event);

    virtual void InitiateDrag(TimelineViewBlockItem* clicked_item,
                              Timeline::MovementMode trim_mode);

    TimelineViewGhostItem* AddGhostFromBlock(Block *block, const TrackReference& track, Timeline::MovementMode mode, bool check_if_exists = false);

    TimelineViewGhostItem* AddGhostFromNull(const rational& in, const rational& out, const TrackReference& track, Timeline::MovementMode mode);

    /**
     * @brief Validates Ghosts that are getting their in points trimmed
     *
     * Assumes ghost->data() is a Block. Ensures no Ghost's in point becomes a negative timecode. Also ensures no
     * Ghost's length becomes 0 or negative.
     */
    rational ValidateInTrimming(rational movement);

    /**
     * @brief Validates Ghosts that are getting their out points trimmed
     *
     * Assumes ghost->data() is a Block. Ensures no Ghost's in point becomes a negative timecode. Also ensures no
     * Ghost's length becomes 0 or negative.
     */
    rational ValidateOutTrimming(rational movement);

    virtual void ProcessDrag(const TimelineCoordinate &mouse_pos);

    void InitiateDragInternal(TimelineViewBlockItem* clicked_item,
                              Timeline::MovementMode trim_mode,
                              bool dont_roll_trims,
                              bool allow_nongap_rolling, bool slide_instead_of_moving);

    const Timeline::MovementMode& drag_movement_mode() const
    {
      return drag_movement_mode_;
    }

    void SetMovementAllowed(bool e)
    {
      movement_allowed_ = e;
    }

    void SetTrimmingAllowed(bool e)
    {
      trimming_allowed_ = e;
    }

    void SetTrackMovementAllowed(bool e)
    {
      track_movement_allowed_ = e;
    }

    void SetGapTrimmingAllowed(bool e)
    {
      gap_trimming_allowed_ = e;
    }

  private:
    Timeline::MovementMode IsCursorInTrimHandle(TimelineViewBlockItem* block, qreal cursor_x);

    void AddGhostInternal(TimelineViewGhostItem* ghost, Timeline::MovementMode mode);

    bool IsClipTrimmable(TimelineViewBlockItem* clip,
                         const QList<TimelineViewBlockItem*>& items,
                         const Timeline::MovementMode& mode);

    void ProcessGhostsForSliding();

    void ProcessGhostsForRolling();

    bool AddMovingTransitionsToClipGhost(Block *block, const TrackReference &track, Timeline::MovementMode movement, const QList<TimelineViewBlockItem *> &selected_items);

    bool movement_allowed_;
    bool trimming_allowed_;
    bool track_movement_allowed_;
    bool gap_trimming_allowed_;
    bool rubberband_selecting_;

    Timeline::TrackType drag_track_type_;
    Timeline::MovementMode drag_movement_mode_;

    TimelineViewBlockItem* clicked_item_;

  };

  class ImportTool : public Tool
  {
  public:
    ImportTool(TimelineWidget* parent);

    virtual void DragEnter(TimelineViewMouseEvent *event) override;
    virtual void DragMove(TimelineViewMouseEvent *event) override;
    virtual void DragLeave(QDragLeaveEvent *event) override;
    virtual void DragDrop(TimelineViewMouseEvent *event) override;

    void PlaceAt(const QList<Footage*> &footage, const rational& start, bool insert);
    void PlaceAt(const QList<DraggedFootage> &footage, const rational& start, bool insert);

  private:
    void FootageToGhosts(rational ghost_start, const QList<DraggedFootage>& footage, const rational &dest_tb, const int &track_start);

    void PrepGhosts(const rational &frame, const int &track_index);

    void DropGhosts(bool insert);

    QList<DraggedFootage> dragged_footage_;

    int import_pre_buffer_;

  };

  class EditTool : public BeamTool
  {
  public:
    EditTool(TimelineWidget* parent);

    virtual void MousePress(TimelineViewMouseEvent *event) override;
    virtual void MouseMove(TimelineViewMouseEvent *event) override;
    virtual void MouseRelease(TimelineViewMouseEvent *event) override;
  };

  class RazorTool : public BeamTool
  {
  public:
    RazorTool(TimelineWidget* parent);

    virtual void MousePress(TimelineViewMouseEvent *event) override;
    virtual void MouseMove(TimelineViewMouseEvent *event) override;
    virtual void MouseRelease(TimelineViewMouseEvent *event) override;

  private:
    QVector<TrackReference> split_tracks_;
  };

  class RippleTool : public PointerTool
  {
  public:
    RippleTool(TimelineWidget* parent);
  protected:
    virtual void FinishDrag(TimelineViewMouseEvent *event) override;

    virtual void InitiateDrag(TimelineViewBlockItem* clicked_item,
                              Timeline::MovementMode trim_mode) override;
  };

  class RollingTool : public PointerTool
  {
  public:
    RollingTool(TimelineWidget* parent);

  protected:
    virtual void InitiateDrag(TimelineViewBlockItem* clicked_item,
                              Timeline::MovementMode trim_mode) override;
  };

  class SlideTool : public PointerTool
  {
  public:
    SlideTool(TimelineWidget* parent);

  protected:
    virtual void InitiateDrag(TimelineViewBlockItem* clicked_item,
                              Timeline::MovementMode trim_mode) override;

  };

  class SlipTool : public PointerTool
  {
  public:
    SlipTool(TimelineWidget* parent);

  protected:
    virtual void ProcessDrag(const TimelineCoordinate &mouse_pos) override;
    virtual void FinishDrag(TimelineViewMouseEvent *event) override;
  };

  class ZoomTool : public Tool
  {
  public:
    ZoomTool(TimelineWidget* parent);

    virtual void MousePress(TimelineViewMouseEvent *event) override;
    virtual void MouseMove(TimelineViewMouseEvent *event) override;
    virtual void MouseRelease(TimelineViewMouseEvent *event) override;

  };

  class AddTool : public BeamTool
  {
  public:
    AddTool(TimelineWidget* parent);

    virtual void MousePress(TimelineViewMouseEvent *event) override;
    virtual void MouseMove(TimelineViewMouseEvent *event) override;
    virtual void MouseRelease(TimelineViewMouseEvent *event) override;

  protected:
    void MouseMoveInternal(const rational& cursor_frame, bool outwards);

    TimelineViewGhostItem* ghost_;

    rational drag_start_point_;
  };

  class TransitionTool : public AddTool
  {
  public:
    TransitionTool(TimelineWidget* parent);

    virtual void MousePress(TimelineViewMouseEvent *event) override;
    virtual void MouseMove(TimelineViewMouseEvent *event) override;
    virtual void MouseRelease(TimelineViewMouseEvent *event) override;
  private:
    bool dual_transition_;
  };

  rational GetToolTipTimebase() const;

  void InsertGapsAt(const rational& time, const rational& length, QUndoCommand* command);

  void ReplaceBlocksWithGaps(const QList<Block *>& blocks, bool remove_from_graph, QUndoCommand* command);

  void SetBlockLinksSelected(Block *block, bool selected);

  QVector<Timeline::EditToInfo> GetEditToInfo(const rational &playhead_time, Timeline::MovementMode mode);

  void RippleTo(Timeline::MovementMode mode);

  void EditTo(Timeline::MovementMode mode);

  void ShowSnap(const QList<rational>& times);

  QPoint drag_origin_;

  void StartRubberBandSelect(bool enable_selecting, bool select_links);
  void MoveRubberBandSelect(bool enable_selecting, bool select_links);
  void EndRubberBandSelect(bool enable_selecting, bool select_links);
  QRubberBand rubberband_;
  QList<QGraphicsItem*> rubberband_now_selected_;

  Tool* GetActiveTool();

  QVector<Tool*> tools_;

  ImportTool* import_tool_;

  Tool* active_tool_;

  void ClearGhosts();

  bool HasGhosts();

  QVector<TimelineViewGhostItem*> ghost_items_;

  QMap<Block*, TimelineViewBlockItem*> block_items_;

  TrackOutput* GetTrackFromReference(const TrackReference& ref);

  void ConnectViewSelectionSignal(TimelineView* view);

  void DisconnectViewSelectionSignal(TimelineView* view);

  QList<TimelineAndTrackView*> views_;

  TimeSlider* timecode_label_;

  int deferred_scroll_value_;

  bool use_audio_time_units_;

  QSplitter* view_splitter_;

  int GetTrackY(const TrackReference& ref);
  int GetTrackHeight(const TrackReference& ref);

  void CenterOn(qreal scene_pos);

  void AddGhost(TimelineViewGhostItem* ghost);

  void UpdateViewTimebases();

  void SetViewBeamCursor(const TimelineCoordinate& coord);

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
  void RemoveBlock(Block* block);

  void AddTrack(TrackOutput* track, Timeline::TrackType type);
  void RemoveTrack(TrackOutput* track);
  void TrackIndexChanged();

  void ViewSelectionChanged();

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

  void ShowSpeedDurationDialog();

  void DeferredScrollAction();

  void ShowSequenceDialog();

  void SetUseAudioTimeUnits(bool use);

  void SetViewTimestamp(const int64_t& ts);

  void ViewTimestampChanged(int64_t ts);

};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINEWIDGET_H
