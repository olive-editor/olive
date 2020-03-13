#ifndef TIMEBASEDWIDGET_H
#define TIMEBASEDWIDGET_H

#include <QWidget>

#include "common/timelinecommon.h"
#include "node/output/viewer/viewer.h"
#include "widget/resizablescrollbar/resizablescrollbar.h"
#include "widget/timelinewidget/timelinescaledobject.h"
#include "widget/timeruler/timeruler.h"

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
  // FIXME: Rename this to SetTimestamp to reduce confusion
  void SetTime(int64_t timestamp);

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

private slots:
  void UpdateMaximumScroll();

  void ScrollBarResized(const double& multiplier);

};

#endif // TIMEBASEDWIDGET_H
