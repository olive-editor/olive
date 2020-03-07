#include "timelinemarker.h"

TimelineMarker::TimelineMarker(const TimeRange &time, const QString &name, QObject *parent) :
  QObject(parent),
  time_(time),
  name_(name)
{
}

const TimeRange &TimelineMarker::time() const
{
  return time_;
}

void TimelineMarker::set_time(const TimeRange &time)
{
  time_ = time;
  emit TimeChanged(time_);
}

const QString &TimelineMarker::name() const
{
  return name_;
}

void TimelineMarker::set_name(const QString &name)
{
  name_ = name;
  emit NameChanged(name_);
}

TimelineMarkerList::~TimelineMarkerList()
{
  qDeleteAll(markers_);
}

void TimelineMarkerList::AddMarker(const TimeRange &time, const QString &name)
{
  TimelineMarker* m = new TimelineMarker(time, name);
  markers_.append(m);
  emit MarkerAdded(m);
}

void TimelineMarkerList::RemoveMarker(TimelineMarker *marker)
{
  for (int i=0;i<markers_.size();i++) {
    TimelineMarker* m = markers_.at(i);

    if (m == marker) {
      markers_.removeAt(i);
      emit MarkerRemoved(m);
      delete m;
      break;
    }
  }
}

const QList<TimelineMarker*> &TimelineMarkerList::list() const
{
  return markers_;
}
