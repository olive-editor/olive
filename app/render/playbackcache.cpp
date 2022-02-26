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

#include "playbackcache.h"

#include "node/output/viewer/viewer.h"
#include "node/project/project.h"
#include "node/project/sequence/sequence.h"
#include "render/diskmanager.h"

namespace olive {

void PlaybackCache::Invalidate(const TimeRange &r, bool signal)
{
  if (r.in() == r.out()) {
    qWarning() << "Tried to invalidate zero-length range";
    return;
  }

  validated_.remove(r);

  InvalidateEvent(r);

  if (signal) {
    emit Invalidated(r);
  }
}

void PlaybackCache::InvalidateAll()
{
  Invalidate(TimeRange(0, RATIONAL_MAX));
}

void PlaybackCache::Shift(rational from, rational to)
{
  if (from == to) {
    return;
  }

  // An region between `from` and `to` will be inserted or spliced out
  TimeRangeList ranges_to_shift = validated_.Intersects(TimeRange(from, RATIONAL_MAX));

  // Remove all ranges starting at to
  validated_.remove(TimeRange(qMin(from, to), RATIONAL_MAX));

  // Restore ranges shifted
  rational diff = to - from;
  foreach (const TimeRange& r, ranges_to_shift) {
    validated_.insert(r + diff);
  }

  // Tell derivatives that a shift has occurred
  ShiftEvent(from, to);

  // Emit signals
  emit Shifted(from, to);

  if (diff > 0) {
    //emit Invalidated(TimeRange(from, to));
  }
}

void PlaybackCache::Validate(const TimeRange &r, bool signal)
{
  validated_.insert(r);

  if (signal) {
    emit Validated(r);
  }
}

void PlaybackCache::InvalidateEvent(const TimeRange &)
{
}

void PlaybackCache::ShiftEvent(const rational &, const rational &)
{
}

Project *PlaybackCache::GetProject() const
{
  // NOTE: A lot of assumptions in this behavior
  ViewerOutput* viewer = static_cast<ViewerOutput*>(parent());
  if (!viewer) {
    return nullptr;
  }

  return viewer->project();
}

TimeRangeList PlaybackCache::GetInvalidatedRanges(TimeRange intersecting)
{
  TimeRangeList invalidated;

  // Prevent TimeRange from being below 0, some other behavior in Olive relies on this behavior
  // and it seemed reasonable to have safety code in here
  intersecting.set_out(qMax(rational(0), intersecting.out()));
  intersecting.set_in(qMax(rational(0), intersecting.in()));

  invalidated.insert(intersecting);

  foreach (const TimeRange &range, validated_) {
    invalidated.remove(range);
  }

  return invalidated;
}

bool PlaybackCache::HasInvalidatedRanges(const TimeRange &intersecting)
{
  return !validated_.contains(intersecting);
}

QString PlaybackCache::GetCacheDirectory() const
{
  Project* project = GetProject();

  if (project) {
    return project->cache_path();
  } else {
    return DiskManager::instance()->GetDefaultCachePath();
  }
}

ViewerOutput *PlaybackCache::viewer_parent() const
{
  return dynamic_cast<ViewerOutput*>(parent());
}

}
