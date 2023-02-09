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

#ifndef RENDERJOBTRACKER_H
#define RENDERJOBTRACKER_H

#include <olive/core/core.h>

#include "common/jobtime.h"

namespace olive {

using namespace core;

class RenderJobTracker
{
public:
  RenderJobTracker() = default;

  void insert(const TimeRange &range, JobTime job_time);
  void insert(const TimeRangeList &ranges, JobTime job_time);

  void clear();

  bool isCurrent(const rational &time, JobTime job_time) const;

  TimeRangeList getCurrentSubRanges(const TimeRange &range, const JobTime &job_time) const;

private:
  class TimeRangeWithJob : public TimeRange
  {
  public:
    TimeRangeWithJob() = default;
    TimeRangeWithJob(const TimeRange &range, const JobTime &job_time)
    {
      set_range(range.in(), range.out());
      job_time_ = job_time;
    }

    JobTime GetJobTime() const {return job_time_;}
    void SetJobTime(JobTime jt) {job_time_ = jt;}

  private:
    JobTime job_time_;

  };

  std::vector<TimeRangeWithJob> jobs_;

};

}

#endif // RENDERJOBTRACKER_H
