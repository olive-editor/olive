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

#include "viewerplaybacktimer.h"

#include <QDateTime>

OLIVE_NAMESPACE_ENTER

void ViewerPlaybackTimer::Start(const int64_t &start_timestamp, const int &playback_speed, const double &timebase)
{
  start_msec_ = QDateTime::currentMSecsSinceEpoch();
  start_timestamp_ = start_timestamp;
  playback_speed_ = playback_speed;
  timebase_ = timebase;
}

int64_t ViewerPlaybackTimer::GetTimestampNow() const
{
  int64_t real_time = QDateTime::currentMSecsSinceEpoch() - start_msec_;

  int64_t frames_since_start = qRound(static_cast<double>(real_time) / (timebase_ * 1000));

  return start_timestamp_ + frames_since_start * playback_speed_;
}

OLIVE_NAMESPACE_EXIT
