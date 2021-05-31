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

#ifndef SAMPLEJOB_H
#define SAMPLEJOB_H

#include "acceleratedjob.h"
#include "codec/samplebuffer.h"

namespace olive {

class SampleJob : public AcceleratedJob {
public:
  SampleJob()
  {
    samples_ = nullptr;
  }

  SampleJob(const NodeValue& value)
  {
    samples_ = value.data().value<SampleBufferPtr>();
  }

  SampleJob(const QString& from, NodeValueDatabase& db)
  {
    samples_ = db[from].Take(NodeValue::kSamples).value<SampleBufferPtr>();
  }

  SampleBufferPtr samples() const
  {
    return samples_;
  }

  bool HasSamples() const
  {
    return samples_ && samples_->is_allocated();
  }

private:
  SampleBufferPtr samples_;

};

}

Q_DECLARE_METATYPE(olive::SampleJob)

#endif // SAMPLEJOB_H
