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

#ifndef GENERATEJOB_H
#define GENERATEJOB_H

#include "acceleratedjob.h"
#include "codec/frame.h"

namespace olive {

class GenerateJob : public AcceleratedJob
{
public:
  typedef void (*GenerateFrameFunction_t)(FramePtr frame, const GenerateJob &job);

  GenerateJob()
  {
    function_ = nullptr;
  }

  GenerateJob(const NodeValueRow &row) :
    GenerateJob()
  {
    Insert(row);
  }

  void do_function(FramePtr frame) const
  {
    if (function_) {
      function_(frame, *this);
    }
  }

  void set_function(GenerateFrameFunction_t function)
  {
    function_ = function;
  }

private:
  GenerateFrameFunction_t function_;

};

}

#endif // GENERATEJOB_H
