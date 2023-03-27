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

#include "globals.h"

namespace olive {

ValueParams ValueParams::time_transformed(const TimeRange &time) const
{
  ValueParams g = *this;
  g.time_ = time;
  return g;
}

ValueParams ValueParams::output_edited(const QString &output) const
{
  ValueParams g = *this;
  g.output_ = output;
  return g;
}

ValueParams ValueParams::loop_mode_edited(const LoopMode &lm) const
{
  ValueParams g = *this;
  g.loop_mode_ = lm;
  return g;
}

}
