/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef NODEGLOBALS_H
#define NODEGLOBALS_H

#include <QVector2D>

#include "common/timerange.h"
#include "render/videoparams.h"

namespace olive {

class NodeGlobals
{
public:
  NodeGlobals(){}

  NodeGlobals(const VideoParams &vparam, const TimeRange &time) :
    video_params_(vparam),
    time_(time)
  {
  }

  QVector2D resolution() const { return video_params_.resolution(); }
  QVector2D resolution_by_par() const { return video_params_.square_resolution(); }
  const VideoParams &video_params() const { return video_params_; }
  const TimeRange &time() const { return time_; }

private:
  VideoParams video_params_;
  TimeRange time_;

};

}

#endif // NODEGLOBALS_H
