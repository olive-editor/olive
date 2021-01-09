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

#ifndef TRACKREFERENCE_H
#define TRACKREFERENCE_H

#include "node/output/track/track.h"
#include "timeline/timelinecommon.h"

namespace olive {

class TrackReference
{
public:
  TrackReference();

  TrackReference(const Track::Type& type, const int& index);

  const Track::Type& type() const;

  const int& index() const;

  bool operator<(const TrackReference& ref) const;

  bool operator==(const TrackReference& ref) const;

  bool operator!=(const TrackReference& ref) const;

private:
  Track::Type type_;

  int index_;

};

uint qHash(const TrackReference& r, uint seed = 0);

}

#endif // TRACKREFERENCE_H
