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
#include "node/globals.h"

namespace olive {

class SampleJob : public AcceleratedJob
{
public:
  SampleJob()
  {
    sample_count_ = 0;
  }

  SampleJob(const ValueParams &p, size_t sample_count)
  {
    value_params_ = p;
    sample_count_ = sample_count;
  }

  SampleJob(const ValueParams &p) :
    SampleJob(p, p.aparams().time_to_samples(p.time().length()))
  {
  }

  const ValueParams &value_params() const { return value_params_; }
  const AudioParams &audio_params() const { return value_params_.aparams(); }
  size_t sample_count() const { return sample_count_; }

private:
  ValueParams value_params_;
  size_t sample_count_;

};

}

Q_DECLARE_METATYPE(olive::SampleJob)

#endif // SAMPLEJOB_H
