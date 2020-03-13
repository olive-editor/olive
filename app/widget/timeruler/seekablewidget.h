#ifndef SEEKABLEWIDGET_H
#define SEEKABLEWIDGET_H

#include "common/rational.h"
#include "timeline/timelinepoints.h"
#include "widget/timelinewidget/view/timelineplayhead.h"
#include "widget/timelinewidget/timelinescaledobject.h"

class SeekableWidget : public TimelineScaledWidget
{
  Q_OBJECT
public:
  SeekableWidget(QWidget *parent = nullptr);

  const int64_t& GetTime() const;

  const int& GetScroll() const;

  void ConnectTimelinePoints(TimelinePoints* points);

public slots:
  void SetTime(const int64_t &r);

  void SetScroll(int s);

protected:
  void SeekToScreenPoint(int screen);

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;

  virtual void ScaleChangedEvent(const double&) override;

  TimelinePoints* timeline_points() const;

  double ScreenToUnitFloat(int screen);

  int64_t ScreenToUnit(int screen);
  int64_t ScreenToUnitRounded(int screen);

  int UnitToScreen(int64_t unit);

  int TimeToScreen(const rational& time);

  inline const QColor& GetPlayheadColor() const
  {
    return style_.GetPlayheadColor();
  }

  inline const QColor& GetPlayheadHighlightColor() const
  {
    return style_.GetPlayheadHighlightColor();
  }

signals:
  /**
   * @brief Signal emitted whenever the time changes on this ruler, either by user or programatically
   */
  void TimeChanged(int64_t);

private:
  int64_t time_;

  TimelinePlayhead style_;

  TimelinePoints* timeline_points_;

  int scroll_;

};

#endif // SEEKABLEWIDGET_H
