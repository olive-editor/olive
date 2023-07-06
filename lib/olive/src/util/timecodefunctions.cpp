/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "util/timecodefunctions.h"

extern "C" {
#include <libavutil/mathematics.h>
}

#include <QRegExp>
#include <QStringList>

namespace olive {

QString leftpad(int64_t t, int count)
{
  return QStringLiteral("%1").arg(qlonglong(t), count, 10, QChar('0'));
}

QString Timecode::time_to_timecode(const rational &time, const rational &timebase, const Timecode::Display &display, bool show_plus_if_positive)
{
  if (timebase.isNull() || timebase.flipped().toDouble() < 1) {
    return "INVALID TIMEBASE";
  }

  double time_dbl = time.toDouble();

  switch (display) {
  case kTimecodeNonDropFrame:
  case kTimecodeDropFrame:
  case kTimecodeSeconds:
  {
    const char *prefix = "";

    if (time_dbl < 0) {
      prefix = "-";
    } else if (show_plus_if_positive) {
      prefix = "+";
    }

    if (display == kTimecodeSeconds) {
      time_dbl = std::abs(time_dbl);

      int64_t total_seconds = std::floor(time_dbl);

      int64_t hours = total_seconds / 3600;
      int64_t mins = total_seconds / 60 - hours * 60;
      int64_t secs = total_seconds - mins * 60;
      int64_t fraction = std::llround((time_dbl - static_cast<double>(total_seconds)) * 1000);

      return QStringLiteral("%1%2:%3:%4.%5").arg(prefix,
                                                 leftpad(hours, 2),
                                                 leftpad(mins, 2),
                                                 leftpad(secs, 2),
                                                 leftpad(fraction, 3));
    } else {
      // Determine what symbol to separate frames (";" is used for drop frame, ":" is non-drop frame)
      const char *frame_token;
      double frame_rate = timebase.flipped().toDouble();
      int rounded_frame_rate = std::llround(frame_rate);
      int64_t frames, secs, mins, hours;
      int64_t f = std::abs(time_to_timestamp(time, timebase));

      if (display == kTimecodeDropFrame && timebase_is_drop_frame(timebase)) {
        frame_token = ";";

        /**
         * CONVERT A FRAME NUMBER TO DROP FRAME TIMECODE
         *
         * Code by David Heidelberger, adapted from Andrew Duncan, further adapted for Olive by Olive Team
         * Given an int called framenumber and a double called framerate
         * Framerate should be 29.97, 59.94, or 23.976, otherwise the calculations will be off.
         */

        // If frame number is greater than 24 hrs, next operation will rollover clock
        f %= (std::llround(frame_rate*3600)*24);

        // Number of frames per ten minutes
        int64_t framesPer10Minutes = std::llround(frame_rate * 600);
        int64_t d = f / framesPer10Minutes;
        int64_t m = f % framesPer10Minutes;

        // Number of frames to drop on the minute marks is the nearest integer to 6% of the framerate
        int64_t dropFrames = std::llround(frame_rate * (2.0/30.0));

        // Number of frames per minute is the round of the framerate * 60 minus the number of dropped frames
        f += dropFrames*9*d;
        if (m > dropFrames) {
          f += dropFrames * ((m - dropFrames) / (std::llround(frame_rate)*60 - dropFrames));
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
                                                  leftpad(hours, 2),
                                                  leftpad(mins, 2),
                                                  leftpad(secs, 2),
                                                  frame_token,
                                                  leftpad(frames, 2));
    }
  }
  case kFrames:
    return QString::number(time_to_timestamp(time, timebase));
  case kMilliseconds:
    return QString::number(std::llround(time_dbl * 1000));
  }

  return "INVALID TIMECODE MODE";
}

int64_t StrToInt64EmptyTolerant(const QString &s, bool *ok)
{
  return s.toLongLong(ok);
}

double StrToDoubleEmptyTolerant(const QString &s, bool *ok)
{
  return s.toDouble(ok);
}

rational Timecode::timecode_to_time(QString timecode, const rational &timebase, const Timecode::Display &display, bool *ok)
{
  timecode = timecode.trimmed();
  if (timecode.isEmpty()) {
    goto err_fatal;
  }

  switch (display) {
  case kTimecodeNonDropFrame:
  case kTimecodeDropFrame:
  case kTimecodeSeconds:
  {
    QStringList timecode_split = timecode.split(QRegExp("(:)|(;)"));

    const int element_count = display == kTimecodeSeconds ? 3 : 4;

    // Remove excess tokens (we're only interested in HH:MM:SS.FF)
    while (timecode_split.size() > element_count) {
      timecode_split.removeLast();
    }

    // For easier index calculations, ensure minimum size
    while (timecode_split.size() < element_count) {
      timecode_split.prepend(QString());
    }

    bool negative = (timecode.at(0) == '-');

    double frame_rate = timebase.flipped().toDouble();
    int rounded_frame_rate = std::lround(frame_rate);

    bool valid;
    rational time;

    int64_t hours = StrToInt64EmptyTolerant(timecode_split.at(0), &valid);
    if (!valid) goto err_fatal;
    int64_t mins = StrToInt64EmptyTolerant(timecode_split.at(1), &valid);
    if (!valid) goto err_fatal;

    if (display == kTimecodeSeconds) {
      double secs = StrToDoubleEmptyTolerant(timecode_split.at(2), &valid);
      if (!valid) goto err_fatal;

      time = rational::fromDouble(hours * 3600 + mins * 60 + secs);
    } else {
      int64_t secs = StrToInt64EmptyTolerant(timecode_split.at(2), &valid);
      if (!valid) goto err_fatal;
      int64_t frames = StrToInt64EmptyTolerant(timecode_split.at(3), &valid);
      if (!valid) goto err_fatal;

      int64_t sec_count = (hours*3600 + mins*60 + secs);
      int64_t frame_count = sec_count*rounded_frame_rate + frames;

      if (display == kTimecodeDropFrame && timebase_is_drop_frame(timebase)) {

        // Number of frames to drop on the minute marks is the nearest integer to 6% of the framerate
        int64_t dropFrames = std::llround(frame_rate * (2.0/30.0));

        // d and m need to be calculated from
        int64_t real_fr_ts = std::llround(static_cast<double>(sec_count)*frame_rate) + frames;

        int64_t framesPer10Minutes = std::llround(frame_rate * 600);
        int64_t d = real_fr_ts / framesPer10Minutes;
        int64_t m = real_fr_ts % framesPer10Minutes;

        if (m > dropFrames) {
          frame_count -= dropFrames * ((m - dropFrames) / (std::llround(frame_rate)*60 - dropFrames));
        }
        frame_count -= dropFrames*9*d;
      }

      time = timestamp_to_time(frame_count, timebase);
    }

    if (ok) *ok = true;

    if (negative) time = -time;

    return time;
  }
  case kMilliseconds:
  {
    double timecode_secs = timecode.toDouble(ok);

    // Convert milliseconds to seconds
    timecode_secs *= 0.001;

    // Convert seconds to rational
    return rational::fromDouble(timecode_secs, ok);
  }
  case kFrames:
    return timecode.toLongLong(ok);
  }

err_fatal:
  if (ok) *ok = false;
  return 0;
}

QString Timecode::time_to_string(int64_t ms)
{
  int64_t total_seconds = ms / 1000;
  int64_t ss = total_seconds % 60;
  int64_t mm = (total_seconds / 60) % 60;
  int64_t hh = total_seconds / 3600;

  return QStringLiteral("%1:%2:%3").arg(leftpad(hh, 2),
                                        leftpad(mm, 2),
                                        leftpad(ss, 2));
}

rational Timecode::snap_time_to_timebase(const rational &time, const rational &timebase, Rounding floor)
{
  // Just convert to a timestamp in timebase units and back
  int64_t timestamp = time_to_timestamp(time, timebase, floor);

  return timestamp_to_time(timestamp, timebase);
}

rational Timecode::timestamp_to_time(const int64_t &timestamp, const rational &timebase)
{
  int64_t num = int64_t(timebase.numerator()) * timestamp;
  int64_t den = timebase.denominator();

  int num_r, den_r;

  av_reduce(&num_r, &den_r, num, den, INT_MAX);

  return rational(num_r, den_r);
}

bool Timecode::timebase_is_drop_frame(const rational &timebase)
{
  return (timebase.numerator() != 1);
}

int64_t Timecode::time_to_timestamp(const rational &time, const rational &timebase, Rounding floor)
{
  return time_to_timestamp(time.toDouble(), timebase, floor);
}

int64_t Timecode::time_to_timestamp(const double &time, const rational &timebase, Rounding floor)
{
  const double d = time * timebase.flipped().toDouble();

  if (std::isnan(d)) {
    return 0;
  }

  const double eps = 0.000000000001;

  switch (floor) {
  case kRound:
  default:
    return std::llround(d);
  case kFloor:
    if (d > std::ceil(d)-eps) {
      return std::ceil(d);
    } else {
      return std::floor(d);
    }
  case kCeil:
    if (d < std::floor(d)+eps) {
      return std::floor(d);
    } else {
      return std::ceil(d);
    }
  }
}

int64_t Timecode::rescale_timestamp(const int64_t &ts, const rational &source, const rational &dest)
{
  if (source == dest) {
    return ts;
  }

  return av_rescale_q(ts, source.toAVRational(), dest.toAVRational());
}

int64_t Timecode::rescale_timestamp_ceil(const int64_t &ts, const rational &source, const rational &dest)
{
  if (source == dest) {
    return ts;
  }

  return av_rescale_q_rnd(ts, source.toAVRational(), dest.toAVRational(), AV_ROUND_UP);
}

}
