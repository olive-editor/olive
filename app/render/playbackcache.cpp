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

void PlaybackCache::Invalidate(const TimeRange &r, qint64 job_time)
{
  if (r.in() == r.out()) {
    qWarning() << "Tried to invalidate zero-length range";
    return;
  }

  invalidated_.insert(r);

  RemoveRangeFromJobs(r);
  jobs_.append({r, job_time});

  InvalidateEvent(r);

  emit Invalidated(r);
}

void PlaybackCache::InvalidateAll()
{
  if (length_.isNull()) {
    return;
  }

  Invalidate(TimeRange(0, length_), 0);
}

void PlaybackCache::SetLength(const rational &r)
{
  if (length_ == r) {
    // Same length - do nothing
    return;
  }

  LengthChangedEvent(length_, r);

  TimeRange range_diff(length_, r);

  if (r.isNull()) {
    invalidated_.clear();
    jobs_.clear();
  } else if (r > length_) {
    // If new length is greater, simply extend the invalidated range for now
    invalidated_.insert(range_diff);
    jobs_.append({range_diff, 0});
  } else {
    // If new length is smaller, removed hashes
    invalidated_.remove(range_diff);
    RemoveRangeFromJobs(range_diff);
  }

  rational old_length = length_;
  length_ = r;

  if (r > old_length) {
    emit Invalidated(range_diff);
  } else {
    emit Validated(range_diff);
  }
}

void PlaybackCache::Shift(rational from, rational to)
{
  if (from == to) {
    return;
  }

  if (from > length_) {
    if (to > from) {
      // No-op
      return;
    } else if (to >= length_) {
      // No-op
      return;
    } else {
      from = length_;
    }
  }

  qDebug() << "FIXME: 0 job time may cause cache desyncs";

  // An region between `from` and `to` will be inserted or spliced out
  TimeRangeList ranges_to_shift = invalidated_.Intersects(TimeRange(from, RATIONAL_MAX));

  // Remove everything from the minimum point
  TimeRange remove_range = TimeRange(qMin(from, to), RATIONAL_MAX);
  RemoveRangeFromJobs(remove_range);
  Validate(remove_range);

  // Shift invalidated ranges
  // (`diff` is POSITIVE when moving forward -> and NEGATIVE when moving backward <-)
  rational diff = to - from;
  foreach (const TimeRange& r, ranges_to_shift) {
    Invalidate(r + diff, 0);
  }

  ShiftEvent(from, to);

  length_ += diff;

  if (diff > 0) {
    // If shifting forward, add this section to the invalidated region
    Invalidate(TimeRange(from, to), 0);
  }

  // Emit signals
  emit Shifted(from, to);
}

void PlaybackCache::Validate(const TimeRange &r)
{
  invalidated_.remove(r);

  emit Validated(r);
}

void PlaybackCache::LengthChangedEvent(const rational &, const rational &)
{
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

void PlaybackCache::RemoveRangeFromJobs(const TimeRange &remove)
{
  // Code shamelessly copied from TimeRangeList::RemoveTimeRange
  for (int i=0;i<jobs_.size();i++) {
    JobIdentifier& job = jobs_[i];
    TimeRange& compare = job.range;

    if (remove.Contains(compare)) {
      // This element is entirely encompassed in this range, remove it
      jobs_.removeAt(i);
      i--;
    } else if (compare.Contains(remove, false, false)) {
      // The remove range is within this element, only choice is to split the element into two
      jobs_.append({TimeRange(remove.out(), compare.out()), job.job_time});
      compare.set_out(remove.in());
    } else if (compare.in() < remove.in() && compare.out() > remove.in()) {
      // This element's out point overlaps the range's in, we'll trim it
      compare.set_out(remove.in());
    } else if (compare.in() < remove.out() && compare.out() > remove.out()) {
      // This element's in point overlaps the range's out, we'll trim it
      compare.set_in(remove.out());
    }
  }
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

}
