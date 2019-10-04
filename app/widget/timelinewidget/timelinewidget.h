#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QScrollBar>
#include <QWidget>

#include "widget/timelineview/timelineview.h"
#include "widget/timeruler/timeruler.h"

/**
 * @brief Full widget for working with TimelineOutput nodes
 *
 * Encapsulates TimelineViews, TimeRulers, and scrollbars for a complete widget to manipulate Timelines
 */
class TimelineWidget : public QWidget
{
  Q_OBJECT
public:
  TimelineWidget(QWidget* parent = nullptr);

  void Clear();

  void SetTime(const int64_t& timestamp);

  void ConnectTimelineNode(TimelineOutput* node);

  void DisconnectTimelineNode();

  void ZoomIn();

  void ZoomOut();

  void SelectAll();

  void DeselectAll();

  void RippleToIn();

  void RippleToOut();

  void EditToIn();

  void EditToOut();

  void GoToPrevCut();

  void GoToNextCut();

public slots:
  void SetTimebase(const rational& timebase);

protected:
  virtual void resizeEvent(QResizeEvent *event) override;

signals:
  void TimeChanged(const int64_t& time);

private:
  enum ViewType {
    kVideo,
    kAudio
  };

  TimelineView* GetView(const ViewType& view_type);

  void RippleEditTo(olive::timeline::MovementMode mode, bool insert_gaps);

  void SetTimeAndSignal(const int64_t& t);

  QVector<TimelineView*> views_;

  TimeRuler* ruler_;

  double scale_;

  TimelineOutput* timeline_node_;

  rational timebase_;

  int64_t playhead_;

  QScrollBar* horizontal_scroll_;

private slots:
  void SetScale(double scale);

  void UpdateInternalTime(const int64_t& timestamp);

};

#endif // TIMELINEWIDGET_H
