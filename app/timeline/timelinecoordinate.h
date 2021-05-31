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

#ifndef TIMELINECOORDINATE_H
#define TIMELINECOORDINATE_H

#include "common/rational.h"
#include "node/output/track/track.h"

namespace olive {

class TimelineCoordinate
{
public:
  TimelineCoordinate();
  TimelineCoordinate(const rational& frame, const Track::Reference& track);
  TimelineCoordinate(const rational& frame, const Track::Type& track_type, const int& track_index);

  const rational& GetFrame() const;
  const Track::Reference& GetTrack() const;

  void SetFrame(const rational& frame);
  void SetTrack(const Track::Reference& track);

private:
  rational frame_;

  Track::Reference track_;

};

}

#endif // TIMELINECOORDINATE_H
