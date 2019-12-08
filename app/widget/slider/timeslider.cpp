#include "timeslider.h"

#include "common/timecodefunctions.h"

TimeSlider::TimeSlider(QWidget *parent) :
  IntegerSlider(parent)
{
  SetMinimum(0);
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

  return olive::timestamp_to_timecode(v.toLongLong(),
                                      timebase_,
                                      olive::CurrentTimecodeDisplay());
}

QVariant TimeSlider::StringToValue(const QString &s, bool *ok)
{
  return olive::timecode_to_timestamp(s, timebase_, olive::CurrentTimecodeDisplay(), ok);
}
