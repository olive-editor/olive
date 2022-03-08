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

#ifndef PLANARPROCESSOR_H
#define PLANARPROCESSOR_H

extern "C" {
#include <libswresample/swresample.h>
}

#include "codec/samplebuffer.h"
#include "render/audioparams.h"

namespace olive {

class PlanarProcessor
{
public:
  PlanarProcessor();

  ~PlanarProcessor();

  DISABLE_COPY_MOVE(PlanarProcessor)

  bool Open(const AudioParams &params);

  SampleBufferPtr Convert(const QByteArray &packed);

  void Close();

  bool IsOpen() const
  {
    return swr_ctx_;
  }

private:
  SwrContext *swr_ctx_;

  AudioParams params_;

};

}

#endif // PLANARPROCESSOR_H
