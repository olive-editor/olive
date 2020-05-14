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

#include "playbackcache.h"

OLIVE_NAMESPACE_ENTER

PlaybackCache::PlaybackCache()
{

}

void PlaybackCache::Invalidate(const TimeRange &r)
{
  invalidated_.InsertTimeRange(r);

  InvalidateEvent(r);

  emit Invalidated(r);
}

void PlaybackCache::SetLength(const rational &r)
{
  if (length_ == r) {
    // Same length - do nothing
    return;
  }

  if (r > length_) {
    // If new length is greater, simply extend the invalidated range for now
    invalidated_.InsertTimeRange(TimeRange(r, length_));
  } else {
    // If new length is smaller, removed hashes
    invalidated_.RemoveTimeRange(TimeRange(length_, r));
  }

  LengthChangedEvent(length_, r);

  length_ = r;
}

bool PlaybackCache::IsFullyValidated() const
{
  return invalidated_.isEmpty();
}

void PlaybackCache::Validate(const TimeRange &r)
{
  invalidated_.RemoveTimeRange(r);

  emit Validated(r);
}

void PlaybackCache::InvalidateAll()
{
  Invalidate(TimeRange(0, length_));
}

void PlaybackCache::LengthChangedEvent(const rational &, const rational &)
{
}

void PlaybackCache::InvalidateEvent(const TimeRange &)
{
}

OLIVE_NAMESPACE_EXIT
