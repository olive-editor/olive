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

#include "renderjobtracker.h"

namespace olive {

void RenderJobTracker::insert(const TimeRange &range, JobTime job_time)
{
  // First remove any ranges with this (code copied
  TimeRangeList::util_remove(&jobs_, range);

  // Now append the job
  TimeRangeWithJob job(range, job_time);
  jobs_.push_back(job);
}

void RenderJobTracker::insert(const TimeRangeList &ranges, JobTime job_time)
{
  foreach (const TimeRange &r, ranges) {
    insert(r, job_time);
  }
}

void RenderJobTracker::clear()
{
  jobs_.clear();
}

bool RenderJobTracker::isCurrent(const rational &time, JobTime job_time) const
{
  for (auto it=jobs_.crbegin(); it!=jobs_.crend(); it++) {
    if (it->Contains(time)) {
      return job_time >= it->GetJobTime();
    }
  }

  return false;
}

TimeRangeList RenderJobTracker::getCurrentSubRanges(const TimeRange &range, const JobTime &job_time) const
{
  TimeRangeList current_ranges;

  for (auto it=jobs_.crbegin(); it!=jobs_.crend(); it++) {
    if (job_time >= it->GetJobTime() && it->OverlapsWith(range)) {
      current_ranges.insert(it->Intersected(range));
    }
  }

  return current_ranges;
}

}
