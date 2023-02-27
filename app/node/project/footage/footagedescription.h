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

#ifndef FOOTAGEDESCRIPTION_H
#define FOOTAGEDESCRIPTION_H

#include "node/output/track/track.h"
#include "render/subtitleparams.h"
#include "render/videoparams.h"

namespace olive {

class FootageDescription
{
public:
  FootageDescription(const QString& decoder = QString()) :
    decoder_(decoder),
    total_stream_count_(0)
  {
  }

  bool IsValid() const
  {
    return !decoder_.isEmpty() && (!video_streams_.isEmpty() || !audio_streams_.isEmpty() || !subtitle_streams_.isEmpty());
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

  void AddSubtitleStream(const SubtitleParams& sub_params)
  {
    Q_ASSERT(!HasStreamIndex(sub_params.stream_index()));

    subtitle_streams_.append(sub_params);
  }

  Track::Type GetTypeOfStream(int index)
  {
    if (StreamIsVideo(index)) {
      return Track::kVideo;
    } else if (StreamIsAudio(index)) {
      return Track::kAudio;
    } else if (StreamIsSubtitle(index)) {
      return Track::kSubtitle;
    } else {
      return Track::kNone;
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

  bool StreamIsSubtitle(int index) const
  {
    foreach (const SubtitleParams& sp, subtitle_streams_) {
      if (sp.stream_index() == index) {
        return true;
      }
    }

    return false;
  }

  bool HasStreamIndex(int index) const
  {
    return StreamIsVideo(index) || StreamIsAudio(index) || StreamIsSubtitle(index);
  }

  int GetStreamCount() const { return total_stream_count_; }
  void SetStreamCount(int s) { total_stream_count_ = s; }

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

  const QVector<SubtitleParams>& GetSubtitleStreams() const
  {
    return subtitle_streams_;
  }

private:
  static constexpr unsigned kFootageMetaVersion = 5;

  QString decoder_;

  QVector<VideoParams> video_streams_;

  QVector<AudioParams> audio_streams_;

  QVector<SubtitleParams> subtitle_streams_;

  int total_stream_count_;

};

}

#endif // FOOTAGEDESCRIPTION_H
