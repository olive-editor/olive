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

#include "sequence.h"

#include "common/channellayout.h"
#include "ui/icons/icons.h"

Sequence::Sequence()
{
  set_icon(olive::icon::Sequence);
}

Item::Type Sequence::type() const
{
  return kSequence;
}

const int &Sequence::video_width()
{
  return video_width_;
}

void Sequence::set_video_width(const int &width)
{
  video_width_ = width;
}

const int &Sequence::video_height() const
{
  return video_height_;
}

void Sequence::set_video_height(const int &height)
{
  video_height_ = height;
}

const rational &Sequence::video_time_base()
{
  return video_time_base_;
}

void Sequence::set_video_time_base(const rational &time_base)
{
  video_time_base_ = time_base;
}

const rational &Sequence::audio_time_base()
{
  return audio_time_base_;
}

void Sequence::set_audio_time_base(const rational &time_base)
{
  audio_time_base_ = time_base;
}

const uint64_t &Sequence::audio_channel_layout()
{
  return audio_channel_layout_;
}

void Sequence::set_audio_channel_layout(const uint64_t &channel_layout)
{
  audio_channel_layout_ = channel_layout;
}

void Sequence::SetDefaultParameters()
{
  // FIXME: Make these configurable
  set_video_width(1920);
  set_video_height(1080);
  set_video_time_base(rational(1001, 30000));

  set_audio_time_base(rational(1, 48000));
  set_audio_channel_layout(AV_CH_LAYOUT_STEREO);
}
