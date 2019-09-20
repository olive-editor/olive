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

#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QGraphicsView>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>

#include "node/block/clip/clip.h"
#include "node/output/timeline/timeline.h"
#include "timelineviewblockitem.h"
#include "timelineviewghostitem.h"
#include "timelineviewplayheaditem.h"

/**
 * @brief A widget for viewing and interacting Sequences
 *
 * This widget primarily exposes users to viewing and modifying Block nodes, usually through a TimelineOutput node.
 */
class TimelineView : public QGraphicsView
{
  Q_OBJECT
public:
  TimelineView(QWidget* parent);

  void SetScale(const double& scale);

  void ConnectTimelineNode(TimelineOutput* node);

  void DisconnectTimelineNode();

public slots:
  void SetTimebase(const rational& timebase);

  void SetTime(const int64_t time);

  void Clear();

  void AddBlock(Block* block, int track);

  void RemoveBlock(Block* block);

  void AddTrack(TrackOutput* track);

  void RemoveTrack(TrackOutput* track);

signals:
  void ScaleChanged(double scale);

  void TimebaseChanged(const rational& timebase);

protected:
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
  virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

  virtual void resizeEvent(QResizeEvent *event) override;

private:

  class Tool
  {
  public:
    Tool(TimelineView* parent);
    virtual ~Tool();

    virtual void MousePress(QMouseEvent *event);
    virtual void MouseMove(QMouseEvent *event);
    virtual void MouseRelease(QMouseEvent *event);

    virtual void DragEnter(QDragEnterEvent *event);
    virtual void DragMove(QDragMoveEvent *event);
    virtual void DragLeave(QDragLeaveEvent *event);
    virtual void DragDrop(QDropEvent *event);

    TimelineView* parent();

    static olive::timeline::MovementMode FlipTrimMode(const olive::timeline::MovementMode& trim_mode);

  protected:
    /**
     * @brief Convert a integer screen point to a float scene point
     *
     * Useful for converting a mouse coordinate provided by a QMouseEvent to a inner-timeline scene position (the
     * coordinates that the graphics items use)
     */
    QPointF GetScenePos(const QPoint& screen_pos);

    /**
     * @brief Retrieve the QGraphicsItem at a particular scene position
     *
     * Requires a float-based scene position. If you have a screen position, use GetScenePos() first to convert it to a
     * scene position
     */
    QGraphicsItem* GetItemAtScenePos(const QPointF& scene_pos);

    /**
     * @brief Validates Ghosts that are moving horizontally (time-based)
     *
     * Validation is the process of ensuring that whatever movements the user is making are "valid" and "legal". This
     * function's validation ensures that no Ghost's in point ends up in a negative timecode.
     */
    rational ValidateFrameMovement(rational movement, const QVector<TimelineViewGhostItem*> ghosts);

    /**
     * @brief Validates Ghosts that are moving vertically (track-based)
     *
     * This function's validation ensures that no Ghost's track ends up in a negative (non-existent) track.
     */
    int ValidateTrackMovement(int movement, const QVector<TimelineViewGhostItem*> ghosts);

    enum SnapPoints {
      kSnapToClips = 0x1,
      kSnapToPlayhead = 0x2,
      kSnapAll = 0xFF
    };

    /**
     * @brief Snaps point `start_point` that is moving by `movement` to currently existing clips
     */
    bool SnapPoint(QList<rational> start_times, rational *movement, int snap_points = kSnapAll);

    QList<rational> snap_points_;

    bool dragging_;

    QPointF drag_start_;

  private:
    TimelineView* parent_;

  };

  class PointerTool : public Tool
  {
  public:
    PointerTool(TimelineView* parent);

    virtual void MousePress(QMouseEvent *event) override;
    virtual void MouseMove(QMouseEvent *event) override;
    virtual void MouseRelease(QMouseEvent *event) override;
  protected:
    void SetMovementAllowed(bool allowed);
    virtual void MouseReleaseInternal(QMouseEvent *event);
    virtual rational FrameValidateInternal(rational time_movement, const QVector<TimelineViewGhostItem *> &ghosts);

    virtual void InitiateGhosts(TimelineViewBlockItem* clicked_item,
                                olive::timeline::MovementMode trim_mode,
                                bool allow_gap_trimming);

    TimelineViewGhostItem* AddGhostFromBlock(Block *block, int track, olive::timeline::MovementMode mode);

    TimelineViewGhostItem* AddGhostFromNull(const rational& in, const rational& out, int track, olive::timeline::MovementMode mode);

    /**
     * @brief Validates Ghosts that are getting their in points trimmed
     *
     * Assumes ghost->data() is a Block. Ensures no Ghost's in point becomes a negative timecode. Also ensures no
     * Ghost's length becomes 0 or negative.
     */
    rational ValidateInTrimming(rational movement, const QVector<TimelineViewGhostItem*> ghosts, bool prevent_overwriting);

    /**
     * @brief Validates Ghosts that are getting their out points trimmed
     *
     * Assumes ghost->data() is a Block. Ensures no Ghost's in point becomes a negative timecode. Also ensures no
     * Ghost's length becomes 0 or negative.
     */
    rational ValidateOutTrimming(rational movement, const QVector<TimelineViewGhostItem*> ghosts, bool prevent_overwriting);
  private:
    void InitiateDrag(const QPoint &mouse_pos);
    void ProcessDrag(const QPoint &mouse_pos);

    void AddGhostInternal(TimelineViewGhostItem* ghost, olive::timeline::MovementMode mode);

    QList<TimelineViewBlockItem*> GetSelectedClips();

    bool IsClipTrimmable(TimelineViewBlockItem* clip,
                         const QList<TimelineViewBlockItem*>& items,
                         const olive::timeline::MovementMode& mode);

    int track_start_;
    bool movement_allowed_;
  };

  class ImportTool : public Tool
  {
  public:
    ImportTool(TimelineView* parent);

    virtual void DragEnter(QDragEnterEvent *event) override;
    virtual void DragMove(QDragMoveEvent *event) override;
    virtual void DragLeave(QDragLeaveEvent *event) override;
    virtual void DragDrop(QDropEvent *event) override;
  private:
    int import_pre_buffer_;
  };

  class RazorTool : public Tool
  {
  public:
    RazorTool(TimelineView* parent);

    virtual void MousePress(QMouseEvent *event);
    virtual void MouseMove(QMouseEvent *event);
    virtual void MouseRelease(QMouseEvent *event);
  };

  class RippleTool : public PointerTool
  {
  public:
    RippleTool(TimelineView* parent);
  protected:
    virtual void MouseReleaseInternal(QMouseEvent *event) override;
    virtual rational FrameValidateInternal(rational time_movement, const QVector<TimelineViewGhostItem*>& ghosts) override;

    virtual void InitiateGhosts(TimelineViewBlockItem* clicked_item,
                                olive::timeline::MovementMode trim_mode,
                                bool allow_gap_trimming) override;
  };

  class HandTool : public Tool
  {
  public:
    HandTool(TimelineView* parent);

    virtual void MousePress(QMouseEvent *event);
    virtual void MouseMove(QMouseEvent *event);
    virtual void MouseRelease(QMouseEvent *event);

  private:
    QPoint screen_drag_start_;
    QPoint scrollbar_start_;
  };

  class ZoomTool : public Tool
  {
  public:
    ZoomTool(TimelineView* parent);

    virtual void MousePress(QMouseEvent *event);
    virtual void MouseMove(QMouseEvent *event);
    virtual void MouseRelease(QMouseEvent *event);
  };

  Tool* GetActiveTool();

  int GetTrackY(int track_index);
  int GetTrackHeight(int track_index);

  PointerTool pointer_tool_;
  ImportTool import_tool_;
  RippleTool ripple_tool_;
  RazorTool razor_tool_;
  HandTool hand_tool_;
  ZoomTool zoom_tool_;

  TimelineOutput* timeline_node_;

  void AddGhost(TimelineViewGhostItem* ghost);

  bool HasGhosts();

  rational SceneToTime(const double &x);
  int SceneToTrack(const double &y);

  void ClearGhosts();

  QGraphicsScene scene_;

  double scale_;

  rational timebase_;

  double timebase_dbl_;

  int64_t playhead_;

  QMap<Block*, TimelineViewRect*> clip_items_;

  QVector<TimelineViewGhostItem*> ghost_items_;

  QVector<int> track_heights_;

  TimelineViewPlayheadItem* playhead_line_;

  Tool* active_tool_;

private slots:
  /**
   * @brief Slot for when a Block node changes its parameters and the graphics need to update
   *
   * This slot does a static_cast on sender() to Block*, meaning all objects triggering this slot must be Blocks or
   * derivatives.
   */
  void BlockChanged();

  /**
   * @brief Slot called whenever the view resizes or the scene contents change to enforce minimum scene sizes
   */
  void UpdateSceneRect();
};

#endif // TIMELINEVIEW_H
