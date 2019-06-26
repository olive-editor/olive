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

#include "audiostream.h"

AudioStream::AudioStream()
{

}

Stream::Type AudioStream::type()
{
  return kAudio;
}

const int &AudioStream::channels()
{
  return channels_;
}

void AudioStream::set_channels(const int &channels)
{
  channels_ = channels;
}

const int &AudioStream::layout()
{
  return layout_;
}

void AudioStream::set_layout(const int &layout)
{
  layout_ = layout;
}

const int &AudioStream::sample_rate()
{
  return sample_rate_;
}

void AudioStream::set_sample_rate(const int &sample_rate)
{
  sample_rate_ = sample_rate;
}
