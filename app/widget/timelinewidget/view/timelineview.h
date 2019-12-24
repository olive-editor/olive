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
#include "timelineviewbase.h"
#include "timelineviewblockitem.h"
#include "timelineviewmouseevent.h"
#include "timelineviewenditem.h"
#include "timelineviewghostitem.h"
#include "widget/timelinewidget/undo/undo.h"
#include "undo/undostack.h"

/**
 * @brief A widget for viewing and interacting Sequences
 *
 * This widget primarily exposes users to viewing and modifying Block nodes, usually through a TimelineOutput node.
 */
class TimelineView : public TimelineViewBase
{
  Q_OBJECT
public:
  TimelineView(const TrackType& type,
               Qt::Alignment vertical_alignment = Qt::AlignTop,
               QWidget* parent = nullptr);

  void SetScale(const double& scale);

  void SelectAll();

  void DeselectAll();

  void SetEndTime(const rational& length);

  int GetTrackY(int track_index);
  int GetTrackHeight(int track_index);

  QPoint GetScrollCoordinates();
  void SetScrollCoordinates(const QPoint& pt);

  void ConnectTrackList(TrackList* list);

signals:
  void ScaleChanged(double scale);

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

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
  virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

  virtual void resizeEvent(QResizeEvent *event) override;

private:
  TrackType ConnectedTrackType();
  Stream::Type TrackTypeToStreamType(TrackType track_type);

  TimelineCoordinate ScreenToCoordinate(const QPoint& pt);
  TimelineCoordinate SceneToCoordinate(const QPointF& pt);

  int SceneToTrack(double y);

  void UserSetTime(const int64_t& time);

  void UpdatePlayheadRect();

  TrackList* connected_track_list_;

  QGraphicsScene scene_;

  TimelineViewEndItem* end_item_;

  TrackType type_;

private slots:
  /**
   * @brief Slot called whenever the view resizes or the scene contents change to enforce minimum scene sizes
   */
  void UpdateSceneRect();

};

#endif // TIMELINEVIEW_H
