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

#include "nodetabletraverser.h"

OLIVE_NAMESPACE_ENTER

QVariant NodeTableTraverser::ProcessVideoFootage(StreamPtr stream, const rational &input_time)
{
  VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(stream);

  return QVariant::fromValue(VideoParams(video_stream->width(),
                                         video_stream->height(),
                                         video_stream->timebase(),
                                         video_stream->format(),
                                         video_stream->pixel_aspect_ratio()));
}

QVariant NodeTableTraverser::ProcessAudioFootage(StreamPtr stream, const TimeRange &input_time)
{
  AudioStreamPtr audio_stream = std::static_pointer_cast<AudioStream>(stream);

  return QVariant::fromValue(AudioParams(audio_stream->sample_rate(),
                                         audio_stream->channel_layout(),
                                         SampleFormat::kInternalFormat));
}

OLIVE_NAMESPACE_EXIT
