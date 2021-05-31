/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef TIMEBASEDWIDGET_H
#define TIMEBASEDWIDGET_H

#include <QWidget>

#include "node/output/viewer/viewer.h"
#include "timeline/timelinecommon.h"
#include "widget/resizablescrollbar/resizabletimelinescrollbar.h"
#include "widget/timebased/timescaledobject.h"
#include "widget/timelinewidget/view/timelineview.h"
#include "widget/timeruler/timeruler.h"

namespace olive {

class TimeBasedWidget : public TimelineScaledWidget
{
  Q_OBJECT
public:
  TimeBasedWidget(bool ruler_text_visible = true, bool ruler_cache_status_visible = false, QWidget* parent = nullptr);

  rational GetTime() const;

  const int64_t& GetTimestamp() const;

  void ZoomIn();

  void ZoomOut();

  ViewerOutput* GetConnectedNode() const;

  void ConnectViewerNode(ViewerOutput *node);

  void SetScaleAndCenterOnPlayhead(const double& scale);

  TimeRuler* ruler() const;

  virtual bool eventFilter(QObject* object, QEvent* event) override;

public slots:
  void SetTimestamp(int64_t timestamp);

  void SetTimebase(const rational& timebase);

  void SetScale(const double& scale);

  void GoToStart();

  void PrevFrame();

  void NextFrame();

  void GoToEnd();

  void GoToPrevCut();

  void GoToNextCut();

  void SetInAtPlayhead();

  void SetOutAtPlayhead();

  void ResetIn();

  void ResetOut();

  void ClearInOutPoints();

  void SetMarker();

  void ToggleShowAll();

  void GoToIn();

  void GoToOut();

protected slots:
  void SetTimeAndSignal(const int64_t& t);

protected:
  ResizableTimelineScrollBar* scrollbar() const;

  virtual void TimebaseChangedEvent(const rational&) override;

  virtual void TimeChangedEvent(const int64_t&){}

  virtual void ScaleChangedEvent(const double &) override;

  virtual void ConnectedNodeChangeEvent(ViewerOutput*){}

  virtual void ConnectNodeEvent(ViewerOutput*){}

  virtual void DisconnectNodeEvent(ViewerOutput*){}

  void SetAutoMaxScrollBar(bool e);

  virtual void resizeEvent(QResizeEvent *event) override;

  void ConnectTimelineView(TimeBasedView* base, bool connect_time_change_event = true);

  void PassWheelEventsToScrollBar(QObject* object);

protected slots:
  /**
   * @brief Slot to center the horizontal scroll bar on the playhead's current position
   */
  void CenterScrollOnPlayhead();

  /**
   * @brief By default, TimeBasedWidget will set the timebase to the viewer node's video timebase.
   * Set this to false if you want to set your own timebase.
   */
  void SetAutoSetTimebase(bool e);

  static void PageScrollInternal(QScrollBar* bar, int maximum, int screen_position, bool whole_page_scroll);

signals:
  void TimeChanged(const int64_t&);

  void TimebaseChanged(const rational&);

  void ConnectedNodeChanged(ViewerOutput* old, ViewerOutput* now);

private:
  class MarkerAddCommand : public UndoCommand
  {
  public:
    MarkerAddCommand(Project* project, TimelineMarkerList* marker_list, const TimeRange& range, const QString& name);

    virtual Project* GetRelevantProject() const override;

    virtual void redo() override;
    virtual void undo() override;

  private:
    Project* project_;
    TimelineMarkerList* marker_list_;
    TimeRange range_;
    QString name_;

    TimelineMarker* added_marker_;

  };

  /**
   * @brief Set either in or out point to the current playhead
   *
   * @param m
   *
   * Set to kTrimIn or kTrimOut for setting the in point or out point respectively.
   */
  void SetPoint(Timeline::MovementMode m, const rational &time);

  /**
   * @brief Reset either the in or out point
   *
   * Sets either the in point to 0 or the out point to `RATIONAL_MAX`.
   *
   * @param m
   *
   * Set to kTrimIn or kTrimOut for setting the in point or out point respectively.
   */
  void ResetPoint(Timeline::MovementMode m);

  void PageScrollInternal(int screen_position, bool whole_page_scroll);

  bool UserIsDraggingPlayhead() const;

  ViewerOutput* viewer_node_;

  TimeRuler* ruler_;

  ResizableTimelineScrollBar* scrollbar_;

  bool auto_max_scrollbar_;

  QList<TimeBasedView*> timeline_views_;

  bool toggle_show_all_;

  double toggle_show_all_old_scale_;
  int toggle_show_all_old_scroll_;

  bool auto_set_timebase_;

  QVector<QObject*> wheel_passthrough_objects_;

  int scrollbar_start_width_;
  double scrollbar_start_value_;
  double scrollbar_start_scale_;
  bool scrollbar_top_handle_;

private slots:
  void UpdateMaximumScroll();

  void ScrollBarResizeBegan(int current_bar_width, bool top_handle);

  void ScrollBarResizeMoved(int new_bar_width);

  /**
   * @brief Slot to handle page scrolling of the playhead
   *
   * If the playhead is outside the current scroll bounds, this function will scroll to where it is. Otherwise it will
   * do nothing.
   */
  void PageScrollToPlayhead();

  void CatchUpScrollToPlayhead();

  void CatchUpScrollToPoint(int point);

  void AutoUpdateTimebase();

  void ConnectedNodeRemovedFromGraph();

};

}

#endif // TIMEBASEDWIDGET_H
