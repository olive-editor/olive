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

#ifndef TIMEBASEDWIDGET_H
#define TIMEBASEDWIDGET_H

#include <QWidget>

#include "common/timelinecommon.h"
#include "node/output/viewer/viewer.h"
#include "widget/resizablescrollbar/resizablescrollbar.h"
#include "widget/timelinewidget/timelinescaledobject.h"
#include "widget/timelinewidget/view/timelineview.h"
#include "widget/timeruler/timeruler.h"

OLIVE_NAMESPACE_ENTER

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

  TimeRuler* ruler() const;

protected slots:
  void SetTimeAndSignal(const int64_t& t);

protected:
  ResizableScrollBar* scrollbar() const;

  virtual void TimebaseChangedEvent(const rational&) override;

  virtual void TimeChangedEvent(const int64_t&){}

  virtual void ScaleChangedEvent(const double &) override;

  virtual void ConnectedNodeChanged(ViewerOutput*){}

  virtual void ConnectNodeInternal(ViewerOutput*){}

  virtual void DisconnectNodeInternal(ViewerOutput*){}

  void SetAutoMaxScrollBar(bool e);

  virtual void resizeEvent(QResizeEvent *event) override;

  virtual TimelinePoints* ConnectTimelinePoints();

  virtual Project* GetTimelinePointsProject();

  TimelinePoints* GetConnectedTimelinePoints() const;

  void ConnectTimelineView(TimelineViewBase* base);

protected slots:
  /**
   * @brief Slot to center the horizontal scroll bar on the playhead's current position
   */
  void CenterScrollOnPlayhead();

signals:
  void TimeChanged(const int64_t&);

  void TimebaseChanged(const rational&);

private:
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

  ViewerOutput* viewer_node_;

  TimeRuler* ruler_;

  ResizableScrollBar* scrollbar_;

  bool auto_max_scrollbar_;

  TimelinePoints* points_;

  QList<TimelineViewBase*> timeline_views_;

  bool toggle_show_all_;

  double toggle_show_all_old_scale_;

private slots:
  void UpdateMaximumScroll();

  void ScrollBarResized(const double& multiplier);

};

OLIVE_NAMESPACE_EXIT

#endif // TIMEBASEDWIDGET_H
