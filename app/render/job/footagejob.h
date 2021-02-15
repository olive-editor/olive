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

#ifndef FOOTAGEJOB_H
#define FOOTAGEJOB_H

#include "project/item/footage/footage.h"

namespace olive {

class FootageJob
{
public:
  FootageJob() = default;

  FootageJob(const Footage::StreamReference& ref, const TimeRange& range) :
    footage_(ref),
    range_(range)
  {
  }

  const Footage::StreamReference& footage() const
  {
    return footage_;
  }

  const TimeRange& range() const
  {
    return range_;
  }

private:
  Footage::StreamReference footage_;

  TimeRange range_;

};

}

Q_DECLARE_METATYPE(olive::FootageJob)

#endif // FOOTAGEJOB_H
