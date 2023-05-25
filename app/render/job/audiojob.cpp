/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "audiojob.h"

#include "acceleratedjob.h"
#include "footagejob.h"
#include "samplejob.h"

namespace olive {

AudioJob::~AudioJob()
{
  delete job_;
}

TimeRange AudioJob::time() const
{
  if (FootageJob *f = dynamic_cast<FootageJob*>(job_)) {
    return f->time();
  } else if (SampleJob *s = dynamic_cast<SampleJob*>(job_)) {
    return s->value_params().time();
  }

  return TimeRange();
}

}
