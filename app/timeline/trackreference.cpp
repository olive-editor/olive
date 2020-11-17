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

#include "trackreference.h"

OLIVE_NAMESPACE_ENTER

TrackReference::TrackReference() :
  type_(Timeline::kTrackTypeNone),
  index_(0)
{
}

TrackReference::TrackReference(const Timeline::TrackType &type, const int &index) :
  type_(type),
  index_(index)
{
}

const Timeline::TrackType &TrackReference::type() const
{
  return type_;
}

const int &TrackReference::index() const
{
  return index_;
}

bool TrackReference::operator==(const TrackReference &ref) const
{
  return type_ == ref.type_ && index_ == ref.index_;
}

bool TrackReference::operator!=(const TrackReference &ref) const
{
  return !(*this == ref);
}

uint qHash(const TrackReference &r, uint seed)
{
  // Not super efficient, but couldn't think of any better way to ensure a different hash each time
  return ::qHash(QStringLiteral("%1:%2").arg(QString::number(r.type()),
                                             QString::number(r.index())),
                 seed);
}

OLIVE_NAMESPACE_EXIT
