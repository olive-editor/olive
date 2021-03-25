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

#ifndef FOOTAGEJOB_H
#define FOOTAGEJOB_H

#include "project/item/footage/footage.h"

namespace olive {

class FootageJob
{
public:
  FootageJob() :
    type_(Stream::kUnknown)
  {
  }

  FootageJob(const QString& decoder, const QString& filename, Stream::Type type) :
    decoder_(decoder),
    filename_(filename),
    type_(type)
  {
  }

  const QString& decoder() const
  {
    return decoder_;
  }

  const QString& filename() const
  {
    return filename_;
  }

  Stream::Type type() const
  {
    return type_;
  }

  const VideoParams& video_params() const
  {
    return video_params_;
  }

  void set_video_params(const VideoParams& p)
  {
    video_params_ = p;
  }

  const AudioParams& audio_params() const
  {
    return audio_params_;
  }

  void set_audio_params(const AudioParams& p)
  {
    audio_params_ = p;
  }

  const QString& cache_path() const
  {
    return cache_path_;
  }

  void set_cache_path(const QString& p)
  {
    cache_path_ = p;
  }

private:
  QString decoder_;

  QString filename_;

  Stream::Type type_;

  VideoParams video_params_;

  AudioParams audio_params_;

  QString cache_path_;

};

}

Q_DECLARE_METATYPE(olive::FootageJob)

#endif // FOOTAGEJOB_H
