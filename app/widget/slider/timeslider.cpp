#include "timeslider.h"

#include "common/timecodefunctions.h"
#include "core.h"

TimeSlider::TimeSlider(QWidget *parent) :
  IntegerSlider(parent)
{
  SetMinimum(0);

  connect(Core::instance(), &Core::TimecodeDisplayChanged, this, &TimeSlider::TimecodeDisplayChanged);
}

void TimeSlider::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;

  // Refresh label since we have a new timebase to generate a timecode with
  UpdateLabel(Value());
}

QString TimeSlider::ValueToString(const QVariant &v)
{
  if (timebase_.isNull()) {
    // We can't generate a timecode without a timebase, so we just return the number
    return IntegerSlider::ValueToString(v);
  }

  return Timecode::timestamp_to_timecode(v.toLongLong(),
                                         timebase_,
                                         Core::instance()->GetTimecodeDisplay());
}

QVariant TimeSlider::StringToValue(const QString &s, bool *ok)
{
  return QVariant::fromValue(Timecode::timecode_to_timestamp(s, timebase_, Core::instance()->GetTimecodeDisplay(), ok));
}

void TimeSlider::TimecodeDisplayChanged()
{
  UpdateLabel(Value());
}
