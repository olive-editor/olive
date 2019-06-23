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

#include "rational.h"

class Footage;

class AudioStream
{
public:
  AudioStream(Footage* footage, const int& channels, const int& layout, const int& sample_rate);

  Footage* footage();
  void set_footage(Footage* f);

  const int& channels();
  void set_channels(const int& channels);

  const int& layout();
  void set_layout(const int& layout);

  const int& sample_rate();
  void set_sample_rate(const int& sample_rate);

private:
  Footage* footage_;

  int channels_;
  int layout_;
  int sample_rate_;
};

#endif // AUDIOSTREAM_H
