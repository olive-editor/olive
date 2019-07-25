#ifndef TIMECODEFUNCTIONS_H
#define TIMECODEFUNCTIONS_H

#include <QString>

#include "common/rational.h"

namespace olive {

enum TimecodeDisplay {
  kTimecodeFrames,
  kTimecodeSeconds,
  kFrames,
  kMilliseconds
};

QString timestamp_to_timecode(const rational& timestamp, const rational& timebase, const TimecodeDisplay& display);

}

#endif // TIMECODEFUNCTIONS_H
