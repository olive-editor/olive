/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "viewerplaybacktimer.h"

#include <QDateTime>
#include <QtMath>

namespace olive {

void ViewerPlaybackTimer::Start(const int64_t &start_timestamp, const int &playback_speed, const double &timebase)
{
  start_msec_ = QDateTime::currentMSecsSinceEpoch();
  start_timestamp_ = start_timestamp;
  playback_speed_ = playback_speed;
  timebase_ = timebase * 1000;
}

int64_t ViewerPlaybackTimer::GetTimestampNow() const
{
  int64_t real_time = QDateTime::currentMSecsSinceEpoch() - start_msec_;

  int64_t frames_since_start = qFloor(static_cast<double>(real_time) / (timebase_));

  return start_timestamp_ + frames_since_start * playback_speed_;
}

}
