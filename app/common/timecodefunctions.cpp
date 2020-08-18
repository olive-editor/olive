/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "timecodefunctions.h"

#include <QtMath>

#include "config/config.h"

OLIVE_NAMESPACE_ENTER

QString padded(int64_t arg, int padding) {
  return QStringLiteral("%1").arg(arg, padding, 10, QChar('0'));
}

QString Timecode::timestamp_to_timecode(const int64_t &timestamp,
                                        const rational& timebase,
                                        const Display& display,
                                        bool show_plus_if_positive)
{
  if (timebase.isNull()) {
    return QStringLiteral("INVALID TIMEBASE");
  }

  double timestamp_dbl = (rational(timestamp) * timebase).toDouble();

  switch (display) {
  case kTimecodeNonDropFrame:
  case kTimecodeDropFrame:
  case kTimecodeSeconds:
  {
    QString prefix;

    if (timestamp_dbl < 0) {
      prefix = "-";
    } else if (show_plus_if_positive) {
      prefix = "+";
    }

    if (display == kTimecodeSeconds) {
      timestamp_dbl = qAbs(timestamp_dbl);

      int64_t total_seconds = qFloor(timestamp_dbl);

      int64_t hours = total_seconds / 3600;
      int64_t mins = total_seconds / 60 - hours * 60;
      int64_t secs = total_seconds - mins * 60;
      int64_t fraction = qRound64((timestamp_dbl - static_cast<double>(total_seconds)) * 1000);

      return QStringLiteral("%1%2:%3:%4.%5").arg(prefix,
                                                 padded(hours, 2),
                                                 padded(mins, 2),
                                                 padded(secs, 2),
                                                 padded(fraction, 3));
    } else {
      // Determine what symbol to separate frames (";" is used for drop frame, ":" is non-drop frame)
      QString frame_token;
      double frame_rate = timebase.flipped().toDouble();
      int rounded_frame_rate = qRound(frame_rate);
      int64_t frames, secs, mins, hours;
      int64_t f = qAbs(timestamp);

      if (display == kTimecodeDropFrame && TimebaseIsDropFrame(timebase)) {
        frame_token = ";";

        /**
         * CONVERT A FRAME NUMBER TO DROP FRAME TIMECODE
         *
         * Code by David Heidelberger, adapted from Andrew Duncan, further adapted for Olive by Olive Team
         * Given an int called framenumber and a double called framerate
         * Framerate should be 29.97, 59.94, or 23.976, otherwise the calculations will be off.
         */

        // If frame number is greater than 24 hrs, next operation will rollover clock
        f %= (qRound(frame_rate*3600)*24);

        // Number of frames per ten minutes
        int64_t framesPer10Minutes = qRound(frame_rate * 600);
        int64_t d = f / framesPer10Minutes;
        int64_t m = f % framesPer10Minutes;

        // Number of frames to drop on the minute marks is the nearest integer to 6% of the framerate
        int64_t dropFrames = qRound(frame_rate * (2.0/30.0));

        // Number of frames per minute is the round of the framerate * 60 minus the number of dropped frames
        f += dropFrames*9*d;
        if (m > dropFrames) {
          f += dropFrames * ((m - dropFrames) / (qRound(frame_rate)*60 - dropFrames));
        }
      } else {
        frame_token = ":";
      }

      // non-drop timecode
      hours = f / (3600*rounded_frame_rate);
      mins = f / (60*rounded_frame_rate) % 60;
      secs = f / rounded_frame_rate % 60;
      frames = f % rounded_frame_rate;

      return QStringLiteral("%1%2:%3:%4%5%6").arg(prefix,
                                                  padded(hours, 2),
                                                  padded(mins, 2),
                                                  padded(secs, 2),
                                                  frame_token,
                                                  padded(frames, 2));
    }
  }
  case kFrames:
    return QString::number(timestamp);
  case kMilliseconds:
    return QString::number(qRound(timestamp_dbl * 1000));
  }

  return QStringLiteral("INVALID TIMECODE MODE");
}

int64_t Timecode::timecode_to_timestamp(const QString &timecode, const rational &timebase, const Display &display, bool* ok)
{
  double timebase_dbl = timebase.toDouble();

  if (timecode.isEmpty()) {
    goto err_fatal;
  }

  switch (display) {
  case kTimecodeNonDropFrame:
  case kTimecodeDropFrame:
  case kTimecodeSeconds:
  {
    const int kTimecodeElementCount = 4;
    QStringList timecode_split = timecode.split(QRegExp("(:)|(;)|(\\.)"));

    bool valid;

    // We only deal with HH, MM, SS, and FF. Any values after that are ignored.
    while (timecode_split.size() > kTimecodeElementCount) {
      timecode_split.removeLast();
    }

    // Convert values to integers
    QList<int64_t> timecode_numbers;

    foreach (const QString& element, timecode_split) {
      valid = true;

      timecode_numbers.append((element.isEmpty()) ? 0 : element.toLong(&valid));

      // If element cannot be converted to a number,
      if (!valid) {
        goto err_fatal;
      }
    }

    // Ensure value size is always 4
    while (timecode_numbers.size() < 4) {
      timecode_numbers.prepend(0);
    }

    double frame_rate = timebase.flipped().toDouble();
    int rounded_frame_rate = qRound(frame_rate);

    int64_t hours = timecode_numbers.at(0);
    int64_t mins = timecode_numbers.at(1);
    int64_t secs = timecode_numbers.at(2);
    int64_t frames = timecode_numbers.at(3);

    int64_t sec_count = (hours*3600 + mins*60 + secs);
    int64_t timestamp = sec_count*rounded_frame_rate + frames;

    if (display == kTimecodeDropFrame && TimebaseIsDropFrame(timebase)) {

      // Number of frames to drop on the minute marks is the nearest integer to 6% of the framerate
      int64_t dropFrames = qRound64(frame_rate * (2.0/30.0));

      // d and m need to be calculated from
      int64_t real_fr_ts = qRound64(static_cast<double>(sec_count)*frame_rate) + frames;

      int64_t framesPer10Minutes = qRound(frame_rate * 600);
      int64_t d = real_fr_ts / framesPer10Minutes;
      int64_t m = real_fr_ts % framesPer10Minutes;

      if (m > dropFrames) {
        timestamp -= dropFrames * ((m - dropFrames) / (qRound(frame_rate)*60 - dropFrames));
      }
      timestamp -= dropFrames*9*d;
    }

    if (ok) *ok = true;
    return timestamp;
  }
  case kMilliseconds:
  {
    bool valid;
    double timecode_secs = timecode.toDouble(&valid);

    if (valid) {
      // Convert milliseconds to seconds
      timecode_secs *= 0.001;

      // Convert seconds to frames
      timecode_secs /= timebase_dbl;

      if (ok) *ok = true;
      return qRound(timecode_secs);
    } else {
      goto err_fatal;
    }
  }
  case kFrames:
    if (ok) *ok = true;
    return timecode.toLong(ok);
  }

err_fatal:
  if (ok) *ok = false;
  return 0;
}

rational Timecode::snap_time_to_timebase(const rational &time, const rational &timebase)
{
  // Just convert to a timestamp in timebase units and back
  int64_t timestamp = time_to_timestamp(time, timebase);

  return timestamp_to_time(timestamp, timebase);
}

rational Timecode::timestamp_to_time(const int64_t &timestamp, const rational &timebase)
{
  return rational(timestamp) * timebase;
}

QString Timecode::time_to_timecode(const rational &time, const rational &timebase, const Timecode::Display &display, bool show_plus_if_positive)
{
  return timestamp_to_timecode(time_to_timestamp(time, timebase), timebase, display, show_plus_if_positive);
}

bool Timecode::TimebaseIsDropFrame(const rational &timebase)
{
  return (timebase.numerator() == 1001);
}

QString Timecode::TimeToString(int64_t ms)
{
  int64_t total_seconds = ms / 1000;
  int64_t ss = total_seconds % 60;
  int64_t mm = (total_seconds / 60) % 60;
  int64_t hh = total_seconds / 3600;

  return QStringLiteral("%1:%2:%3")
      .arg(hh, 2, 10, QChar('0'))
      .arg(mm, 2, 10, QChar('0'))
      .arg(ss, 2, 10, QChar('0'));
}

int64_t Timecode::time_to_timestamp(const rational &time, const rational &timebase)
{
  return time_to_timestamp(time.toDouble(), timebase);
}

int64_t Timecode::time_to_timestamp(const double &time, const rational &timebase)
{
  return qRound64(time * timebase.flipped().toDouble());
}

int64_t Timecode::rescale_timestamp(const int64_t &ts, const rational &source, const rational &dest)
{
  return qRound64(static_cast<double>(ts) * source.toDouble() / dest.toDouble());
}

int64_t Timecode::rescale_timestamp_ceil(const int64_t &ts, const rational &source, const rational &dest)
{
  return qCeil(static_cast<double>(ts) * source.toDouble() / dest.toDouble());
}

OLIVE_NAMESPACE_EXIT
