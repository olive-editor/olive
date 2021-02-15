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

#ifndef RENDERTASK_H
#define RENDERTASK_H

#include <QtConcurrent/QtConcurrent>

#include "node/output/viewer/viewer.h"
#include "render/colormanager.h"
#include "task/task.h"
#include "threading/threadticket.h"
#include "threading/threadticketwatcher.h"

namespace olive {

class RenderTask : public Task
{
  Q_OBJECT
public:
  RenderTask(Sequence* viewer, const VideoParams &vparams, const AudioParams &aparams);

  virtual ~RenderTask() override;

protected:
  bool Render(ColorManager *manager, const TimeRangeList &video_range,
              const TimeRangeList &audio_range, RenderMode::Mode mode,
              FrameHashCache *cache, const QSize& force_size = QSize(0, 0),
              const QMatrix4x4& force_matrix = QMatrix4x4(),
              VideoParams::Format force_format = VideoParams::kFormatInvalid,
              ColorProcessorPtr force_color_output = nullptr);

  virtual void DownloadFrame(QThread* thread, FramePtr frame, const QByteArray &hash);

  virtual void FrameDownloaded(FramePtr frame, const QByteArray& hash, const QVector<rational>& times, qint64 job_time) = 0;

  virtual void AudioDownloaded(const TimeRange& range, SampleBufferPtr samples, qint64 job_time) = 0;

  Sequence* viewer() const
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

  virtual void CancelEvent() override
  {
    finished_watcher_mutex_.lock();
    finished_watcher_wait_cond_.wakeAll();
    finished_watcher_mutex_.unlock();
  }

  virtual bool TwoStepFrameRendering() const
  {
    return true;
  }

private:
  void PrepareWatcher(RenderTicketWatcher* watcher, QThread *thread);

  void IncrementRunningTickets();

  Sequence* viewer_;

  VideoParams video_params_;

  AudioParams audio_params_;

  QVector<RenderTicketWatcher*> running_watchers_;
  std::list<RenderTicketWatcher*> finished_watchers_;
  int running_tickets_;
  QMutex finished_watcher_mutex_;
  QWaitCondition finished_watcher_wait_cond_;

private slots:
  void TicketDone(RenderTicketWatcher *watcher);

};

}

#endif // RENDERTASK_H
