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

#ifndef RENDERTASK_H
#define RENDERTASK_H

#include <QtConcurrent/QtConcurrent>

#include "node/output/viewer/viewer.h"
#include "task/task.h"

OLIVE_NAMESPACE_ENTER

class RenderTask : public Task
{
public:
  RenderTask(ViewerOutput* viewer, const VideoParams &vparams, const AudioParams &aparams);

  virtual ~RenderTask() override;

protected:
  void Render(const TimeRangeList &video_range,
              const TimeRangeList &audio_range, RenderMode::Mode mode,
              bool use_disk_cache);

  virtual QFuture<void> DownloadFrame(FramePtr frame, const QByteArray &hash) = 0;

  virtual void FrameDownloaded(const QByteArray& hash, const std::list<rational>& times, qint64 job_time) = 0;

  virtual void AudioDownloaded(const TimeRange& range, SampleBufferPtr samples, qint64 job_time) = 0;

  ViewerOutput* viewer() const
  {
    return viewer_;
  }

  const VideoParams& video_params() const
  {
    return video_params_;
  }

  const AudioParams& audio_params() const
  {
    return audio_params_;
  }

private:
  ViewerOutput* viewer_;

  VideoParams video_params_;

  AudioParams audio_params_;

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERTASK_H
