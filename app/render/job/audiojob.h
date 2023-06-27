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

#ifndef AUDIOJOB_H
#define AUDIOJOB_H

#include <memory>

#include "common/define.h"
#include "render/audioparams.h"
#include "util/timerange.h"

namespace olive {

class AcceleratedJob;

class AudioJob;
using AudioJobPtr = std::shared_ptr<AudioJob>;

class AudioJob
{
public:
  AudioJob()
  {
    job_ = nullptr;
  }

  template <typename T>
  AudioJob(const AudioParams &params, const T &job)
  {
    job_ = new T(job);
    params_ = params;
  }

  template <typename T>
  static AudioJobPtr Create(const AudioParams &params, const T &job)
  {
    return std::make_shared<AudioJob>(params, job);
  }

  ~AudioJob();

  DISABLE_COPY_MOVE(AudioJob)

  AcceleratedJob *job() const { return job_; }
  const AudioParams &params() const { return params_; }

  TimeRange time() const;

private:
  AudioParams params_;
  AcceleratedJob *job_;

};

}

#endif // AUDIOJOB_H
