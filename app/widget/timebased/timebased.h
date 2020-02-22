#ifndef TIMEBASEDWIDGET_H
#define TIMEBASEDWIDGET_H

#include <QScrollBar>
#include <QWidget>

#include "node/output/viewer/viewer.h"
#include "widget/timelinewidget/timelinescaledobject.h"
#include "widget/timeruler/timeruler.h"

class TimeBasedWidget : public QWidget, public TimelineScaledObject
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

  TimeRuler* ruler() const;

protected slots:
  void SetTimeAndSignal(const int64_t& t);

protected:
  QScrollBar* scrollbar() const;

  virtual void TimebaseChangedEvent(const rational&) override;

  virtual void TimeChangedEvent(const int64_t&){}

  virtual void ScaleChangedEvent(const double &) override;

  virtual void ConnectedNodeChanged(ViewerOutput*){}

  virtual void ConnectNodeInternal(ViewerOutput*){}

  virtual void DisconnectNodeInternal(ViewerOutput*){}

  void SetAutoMaxScrollBar(bool e);

  virtual void resizeEvent(QResizeEvent *event) override;

protected slots:
  /**
   * @brief Slot to center the horizontal scroll bar on the playhead's current position
   */
  void CenterScrollOnPlayhead();

signals:
  void TimeChanged(const int64_t&);

  void TimebaseChanged(const rational&);

private:
  ViewerOutput* viewer_node_;

  TimeRuler* ruler_;

  QScrollBar* scrollbar_;

  bool auto_max_scrollbar_;

private slots:
  void UpdateMaximumScroll();

};

#endif // TIMEBASEDWIDGET_H
