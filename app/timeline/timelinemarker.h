#ifndef TIMELINEMARKER_H
#define TIMELINEMARKER_H

#include <QString>

#include "common/timerange.h"

class TimelineMarker : public QObject
{
  Q_OBJECT
public:
  TimelineMarker(const TimeRange& time = TimeRange(), const QString& name = QString(), QObject* parent = nullptr);

  const TimeRange &time() const;
  void set_time(const TimeRange& time);

  const QString& name() const;
  void set_name(const QString& name);

signals:
  void TimeChanged(const TimeRange& time);

  void NameChanged(const QString& name);

private:
  TimeRange time_;

  QString name_;

};

class TimelineMarkerList : public QObject
{
  Q_OBJECT
public:
  TimelineMarkerList() = default;

  virtual ~TimelineMarkerList() override;

  void AddMarker(const TimeRange& time = TimeRange(), const QString& name = QString());

  void RemoveMarker(TimelineMarker* marker);

  const QList<TimelineMarker *> &list() const;

signals:
  void MarkerAdded(TimelineMarker* marker);

  void MarkerRemoved(TimelineMarker* marker);

private:
  QList<TimelineMarker*> markers_;

};

#endif // TIMELINEMARKER_H
