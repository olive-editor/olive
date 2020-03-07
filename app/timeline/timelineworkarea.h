#ifndef TIMELINEWORKAREA_H
#define TIMELINEWORKAREA_H

#include <QObject>

#include "common/timerange.h"

class TimelineWorkArea : public QObject
{
  Q_OBJECT
public:
  TimelineWorkArea(QObject* parent = nullptr);

  bool enabled() const;
  void set_enabled(bool e);

  const rational& in() const;
  const rational& out() const;
  const TimeRange& range() const;
  void set_range(const TimeRange& range);

  static const rational kResetIn;
  static const rational kResetOut;

signals:
  void EnabledChanged(bool e);

  void RangeChanged(const TimeRange& r);

private:
  bool workarea_enabled_;

  TimeRange workarea_range_;

};

#endif // TIMELINEWORKAREA_H
