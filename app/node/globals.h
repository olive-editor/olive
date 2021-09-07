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

#ifndef NODEGLOBALS_H
#define NODEGLOBALS_H

#include <QVector2D>

#include "common/timerange.h"

namespace olive {

class NodeGlobals
{
public:
  NodeGlobals(const QVector2D &resolution, const TimeRange &time) :
    resolution_(resolution),
    time_(time)
  {}

  const QVector2D &resolution() const
  {
    return resolution_;
  }

  const TimeRange &time() const
  {
    return time_;
  }

  void set_time(const TimeRange &time)
  {
    time_ = time;
  }

private:
  QVector2D resolution_;

  TimeRange time_;

};

}

#endif // NODEGLOBALS_H
