#include "timecodefunctions.h"

#include <QtMath>

QString padded(int arg, int padding) {
  return QString("%1").arg(arg, padding, 10, QChar('0'));
}

QString olive::timestamp_to_timecode(const rational& timestamp,
                                     const rational& timebase,
                                     const TimecodeDisplay& display)
{
  double timestamp_dbl = timestamp.ToDouble();



  switch (display) {
  case kTimecodeFrames:
  case kTimecodeSeconds:
  {
    int total_seconds = qFloor(timestamp_dbl);

    int hours = total_seconds / 3600;
    int mins = total_seconds / 60 - hours * 60;
    int secs = total_seconds - mins * 60;

    if (display == kTimecodeSeconds) {
      int fraction = qRound((timestamp_dbl - total_seconds) * 1000);

      return QString("%1:%2:%3.%4").arg(padded(hours, 2),
                                        padded(mins, 2),
                                        padded(secs, 2),
                                        padded(fraction, 3));
    } else {
      rational frame_rate = timebase.flipped();

      int frames = qRound((timestamp_dbl - total_seconds) * frame_rate.ToDouble());

      return QString("%1:%2:%3;%4").arg(padded(hours, 2),
                                        padded(mins, 2),
                                        padded(secs, 2),
                                        padded(frames, 2));
    }
  }
  case kFrames:
    return QString::number(timestamp.numerator() / timebase.numerator());
  case kMilliseconds:
    return QString::number(qFloor(timestamp_dbl * 1000));
  }

  return QString();
}
