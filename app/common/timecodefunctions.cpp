#include "timecodefunctions.h"

#include <QtMath>

QString padded(int arg, int padding) {
  return QString("%1").arg(arg, padding, 10, QChar('0'));
}

QString olive::timestamp_to_timecode(const rational &timestamp)
{
  double timestamp_dbl = timestamp.ToDouble();

  int total_seconds = qFloor(timestamp_dbl);

  int hours = total_seconds / 3600;
  int mins = total_seconds / 60 - hours * 60;
  int secs = total_seconds - mins * 60;
  int fraction = qRound((timestamp_dbl - total_seconds) * 1000);

  return QString("%1:%2:%3.%4").arg(padded(hours, 2),
                                    padded(mins, 2),
                                    padded(secs, 2),
                                    padded(fraction, 3));
}
