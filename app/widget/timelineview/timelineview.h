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
#include "timelineviewclipitem.h"
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

  void AddBlock(Block* block, int track);

  void RemoveBlock(Block* block);

  void RemoveBlocksOfTrack(Block* block);

  void SetScale(const double& scale);

  void SetTimebase(const rational& timebase);

  void Clear();

public slots:
  void SetTime(const int64_t time);

signals:
  void RequestPlaceBlock(Block* block, rational start, int track);

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

  protected:
    QPointF GetScenePos(const QPoint& screen_pos);

    QGraphicsItem* GetItemAtScenePos(const QPointF& scene_pos);

    bool dragging_;

    QPointF drag_start_;

  private:
    TimelineView* parent_;

  };

  class PointerTool : public Tool
  {
  public:
    PointerTool(TimelineView* parent);

    virtual void MousePress(QMouseEvent *event);
    virtual void MouseMove(QMouseEvent *event);
    virtual void MouseRelease(QMouseEvent *event);
  };

  class ImportTool : public Tool
  {
  public:
    ImportTool(TimelineView* parent);

    virtual void DragEnter(QDragEnterEvent *event) override;
    virtual void DragMove(QDragMoveEvent *event) override;
    virtual void DragLeave(QDragLeaveEvent *event) override;
    virtual void DragDrop(QDropEvent *event) override;
  };

  int GetTrackY(int track_index);
  int GetTrackHeight(int track_index);

  PointerTool pointer_tool_;
  ImportTool import_tool_;

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
