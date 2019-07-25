#ifndef TIMECODEFUNCTIONS_H
#define TIMECODEFUNCTIONS_H

#include <QString>

#include "common/rational.h"

namespace olive {

QString timestamp_to_timecode(const rational& timestamp);

}

#endif // TIMECODEFUNCTIONS_H
