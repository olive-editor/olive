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

#ifndef SAMPLEJOB_H
#define SAMPLEJOB_H

#include "acceleratedjob.h"

namespace olive {

class SampleJob : public AcceleratedJob
{
public:
  SampleJob()
  {
  }

  SampleJob(const TimeRange &time, const NodeValue& value)
  {
    samples_ = value.toSamples();
    time_ = time;
  }

  SampleJob(const TimeRange &time, const QString& from, const NodeValueRow& row)
  {
    samples_ = row[from].toSamples();
    time_ = time;
  }

  const SampleBuffer &samples() const
  {
    return samples_;
  }

  bool HasSamples() const
  {
    return samples_.is_allocated();
  }

  const TimeRange &time() const { return time_; }

private:
  SampleBuffer samples_;

  TimeRange time_;

};

}

Q_DECLARE_METATYPE(olive::SampleJob)

#endif // SAMPLEJOB_H
