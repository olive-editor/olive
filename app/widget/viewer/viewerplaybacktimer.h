/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef VIEWERPLAYBACKTIMER_H
#define VIEWERPLAYBACKTIMER_H

#include <QtGlobal>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class ViewerPlaybackTimer {
public:
  void Start(const int64_t& start_timestamp, const int& playback_speed, const double& timebase);

  int64_t GetTimestampNow() const;

private:
  qint64 start_msec_;
  int64_t start_timestamp_;

  int playback_speed_;

  double timebase_;

};

OLIVE_NAMESPACE_EXIT

#endif // VIEWERPLAYBACKTIMER_H
