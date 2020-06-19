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

#include <QDateTime>

OLIVE_NAMESPACE_ENTER

void PlaybackCache::Invalidate(const TimeRange &r)
{
  QMutexLocker locker(lock());

  NoLockInvalidate(r);

  locker.unlock();

  emit Invalidated(r);
}

void PlaybackCache::InvalidateAll()
{
  QMutexLocker locker(lock());

  if (length_.isNull()) {
    return;
  }

  TimeRange invalidate_range(0, length_);

  NoLockInvalidate(invalidate_range);

  locker.unlock();

  emit Invalidated(invalidate_range);
}

void PlaybackCache::SetLength(const rational &r)
{
  QMutexLocker locker(lock());

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
    invalidated_.InsertTimeRange(range_diff);
    jobs_.append({range_diff, QDateTime::currentMSecsSinceEpoch()});
  } else {
    // If new length is smaller, removed hashes
    invalidated_.RemoveTimeRange(range_diff);
    RemoveRangeFromJobs(range_diff);
  }

  rational old_length = length_;
  length_ = r;

  locker.unlock();

  if (r > old_length) {
    emit Invalidated(range_diff);
  } else {
    emit Validated(range_diff);
  }
}

void PlaybackCache::Shift(const rational &from, const rational &to)
{
  if (from == to) {
    return;
  }

  QMutexLocker locker(lock());

  // An region between `from` and `to` will be inserted or spliced out
  TimeRangeList ranges_to_shift = invalidated_.Intersects(TimeRange(from, RATIONAL_MAX));

  // Remove everything from the minimum point
  TimeRange remove_range = TimeRange(qMin(from, to), RATIONAL_MAX);
  NoLockValidate(remove_range);
  RemoveRangeFromJobs(remove_range);

  // Shift everything in our ranges to shift list
  // (`diff` is POSITIVE when moving forward -> and NEGATIVE when moving backward <-)
  rational diff = to - from;
  foreach (const TimeRange& r, ranges_to_shift) {
    NoLockInvalidate(r + diff);
  }

  ShiftEvent(from, to);

  length_ += diff;

  if (diff > rational()) {
    // If shifting forward, add this section to the invalidated region
    NoLockInvalidate(TimeRange(from, to));
  }

  locker.unlock();

  // Emit signals
  emit Validated(remove_range);
  foreach (const TimeRange& r, ranges_to_shift) {
    emit Invalidated(r + diff);
  }
  if (diff > rational()) {
    emit Invalidated(TimeRange(from, to));
  }
  emit Shifted(from, to);
}

void PlaybackCache::NoLockInvalidate(const TimeRange &r)
{
  Q_ASSERT(r.in() != r.out());

  invalidated_.InsertTimeRange(r);

  RemoveRangeFromJobs(r);
  qint64 job_time = QDateTime::currentMSecsSinceEpoch();
  jobs_.append({r, job_time});

  InvalidateEvent(r);
}

void PlaybackCache::NoLockValidate(const TimeRange &r)
{
  invalidated_.RemoveTimeRange(r);
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

OLIVE_NAMESPACE_EXIT
