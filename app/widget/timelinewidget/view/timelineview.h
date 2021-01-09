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

#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QGraphicsView>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>

#include "node/block/clip/clip.h"
#include "timelineviewmouseevent.h"
#include "timelineviewghostitem.h"
#include "widget/timebased/timebasedview.h"
#include "widget/timelinewidget/undo/undo.h"
#include "undo/undostack.h"

namespace olive {

/**
 * @brief A widget for viewing and interacting Sequences
 *
 * This widget primarily exposes users to viewing and modifying Block nodes, usually through a TimelineOutput node.
 */
class TimelineView : public TimeBasedView
{
  Q_OBJECT
public:
  TimelineView(Qt::Alignment vertical_alignment = Qt::AlignTop,
               QWidget* parent = nullptr);

  int GetTrackY(int track_index) const;
  int GetTrackHeight(int track_index) const;

  QPoint GetScrollCoordinates() const;
  void SetScrollCoordinates(const QPoint& pt);

  void ConnectTrackList(TrackList* list);

  void SetBeamCursor(const TimelineCoordinate& coord);

  void SetSelectionList(QHash<TrackReference, TimeRangeList>* s)
  {
    selections_ = s;
  }

  void SetGhostList(QVector<TimelineViewGhostItem*>* ghosts)
  {
    ghosts_ = ghosts;
  }

signals:
  void MousePressed(TimelineViewMouseEvent* event);
  void MouseMoved(TimelineViewMouseEvent* event);
  void MouseReleased(TimelineViewMouseEvent* event);
  void MouseDoubleClicked(TimelineViewMouseEvent* event);

  void DragEntered(TimelineViewMouseEvent* event);
  void DragMoved(TimelineViewMouseEvent* event);
  void DragLeft(QDragLeaveEvent* event);
  void DragDropped(TimelineViewMouseEvent* event);

protected:
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

  virtual void wheelEvent(QWheelEvent* event) override;

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
  virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

  virtual void drawBackground(QPainter *painter, const QRectF &rect) override;
  virtual void drawForeground(QPainter *painter, const QRectF &rect) override;

  virtual void ToolChangedEvent(Tool::Item tool) override;

  virtual void SceneRectUpdateEvent(QRectF& rect) override;

private:
  Track::Type ConnectedTrackType();
  Stream::Type TrackTypeToStreamType(Track::Type track_type);

  TimelineCoordinate ScreenToCoordinate(const QPoint& pt);
  TimelineCoordinate SceneToCoordinate(const QPointF& pt);

  TimelineViewMouseEvent CreateMouseEvent(QMouseEvent* event);
  TimelineViewMouseEvent CreateMouseEvent(const QPoint &pos, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);

  int GetHeightOfAllTracks() const;

  int SceneToTrack(double y);

  void UserSetTime(const int64_t& time);

  void UpdatePlayheadRect();

  QHash<TrackReference, TimeRangeList>* selections_;

  QVector<TimelineViewGhostItem*>* ghosts_;

  bool show_beam_cursor_;

  TimelineCoordinate cursor_coord_;

  TrackList* connected_track_list_;

};

}

#endif // TIMELINEVIEW_H
