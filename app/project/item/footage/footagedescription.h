/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef FOOTAGEDESCRIPTION_H
#define FOOTAGEDESCRIPTION_H

#include "render/audioparams.h"
#include "render/videoparams.h"
#include "stream.h"

namespace olive {

class FootageDescription
{
public:
  FootageDescription(const QString& decoder = QString()) :
    decoder_(decoder)
  {
  }

  bool IsValid() const
  {
    return !decoder_.isEmpty() && (!video_streams_.isEmpty() || !audio_streams_.isEmpty());
  }

  const QString& decoder() const
  {
    return decoder_;
  }

  void AddVideoStream(const VideoParams& video_params)
  {
    Q_ASSERT(!HasStreamIndex(video_params.stream_index()));

    video_streams_.append(video_params);
  }

  void AddAudioStream(const AudioParams& audio_params)
  {
    Q_ASSERT(!HasStreamIndex(audio_params.stream_index()));

    audio_streams_.append(audio_params);
  }

  Stream::Type GetTypeOfStream(int index)
  {
    if (StreamIsVideo(index)) {
      return Stream::kVideo;
    } else if (StreamIsAudio(index)) {
      return Stream::kAudio;
    } else {
      return Stream::kUnknown;
    }
  }

  bool StreamIsVideo(int index) const
  {
    foreach (const VideoParams& vp, video_streams_) {
      if (vp.stream_index() == index) {
        return true;
      }
    }

    return false;
  }

  bool StreamIsAudio(int index) const
  {
    foreach (const AudioParams& ap, audio_streams_) {
      if (ap.stream_index() == index) {
        return true;
      }
    }

    return false;
  }

  bool HasStreamIndex(int index) const
  {
    return StreamIsVideo(index) || StreamIsAudio(index);
  }

  bool Load(const QString& filename);

  bool Save(const QString& filename) const;

  const QVector<VideoParams>& GetVideoStreams() const
  {
    return video_streams_;
  }

  const QVector<AudioParams>& GetAudioStreams() const
  {
    return audio_streams_;
  }

private:
  QString decoder_;

  QVector<VideoParams> video_streams_;

  QVector<AudioParams> audio_streams_;

};

}

#endif // FOOTAGEDESCRIPTION_H
