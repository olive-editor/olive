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

#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "rational.h"
#include "stream.h"

/**
 * @brief A Stream derivative containing audio-specific information
 */
class AudioStream : public Stream
{
public:
  AudioStream();

  const int& channels();
  void set_channels(const int& channels);

  const uint64_t& layout();
  void set_layout(const uint64_t& layout);

  const int& sample_rate();
  void set_sample_rate(const int& sample_rate);

private:
  int channels_;
  uint64_t layout_;
  int sample_rate_;
};

#endif // AUDIOSTREAM_H
