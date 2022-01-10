/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "node/block/subtitle/subtitle.h"
#include "node/color/colormanager/colormanager.h"
#include "node/output/viewer/viewer.h"
#include "task/task.h"
#include "threading/threadticket.h"
#include "threading/threadticketwatcher.h"

namespace olive {

class RenderTask : public Task
{
  Q_OBJECT
public:
  RenderTask();

  virtual ~RenderTask() override;

protected:
  bool Render(ColorManager *manager, const TimeRangeList &video_range,
              const TimeRangeList &audio_range, const TimeRange &subtitle_range,
              RenderMode::Mode mode,
              FrameHashCache *cache, const QSize& force_size = QSize(0, 0),
              const QMatrix4x4& force_matrix = QMatrix4x4(),
              VideoParams::Format force_format = VideoParams::kFormatInvalid,
              ColorProcessorPtr force_color_output = nullptr);

  virtual bool DownloadFrame(QThread* thread, FramePtr frame, const QByteArray &hash);

  virtual bool FrameDownloaded(FramePtr frame, const QByteArray& hash, const QVector<rational>& times) = 0;

  virtual bool AudioDownloaded(const TimeRange& range, SampleBufferPtr samples) = 0;

  virtual bool EncodeSubtitle(const SubtitleBlock *subtitle);

  ViewerOutput* viewer() const
  {
    return viewer_;
  }

  void set_viewer(ViewerOutput *v)
  {
    viewer_ = v;
  }

  const VideoParams& video_params() const
  {
    return video_params_;
  }

  void set_video_params(const VideoParams& video_params)
  {
    video_params_ = video_params;
  }

  const AudioParams& audio_params() const
  {
    return audio_params_;
  }

  void set_audio_params(const AudioParams& audio_params)
  {
    audio_params_ = audio_params;
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

  void SetNativeProgressSignallingEnabled(bool e)
  {
    native_progress_signalling_ = e;
  }

  /**
   * @brief Only valid after Render() is called
   */
  int64_t GetTotalNumberOfFrames() const
  {
    return total_number_of_frames_;
  }

  /**
   * @brief Only valid after Render() is called
   */
  int64_t GetTotalNumberOfUniqueFrames() const
  {
    return total_number_of_unique_frames_;
  }

private:
  void PrepareWatcher(RenderTicketWatcher* watcher, QThread *thread);

  void IncrementRunningTickets();

  void StartTicket(const QByteArray &hash, QThread *watcher_thread, ColorManager *manager, const rational &time, RenderMode::Mode mode, FrameHashCache *cache, const QSize &force_size, const QMatrix4x4 &force_matrix, VideoParams::Format force_format, ColorProcessorPtr force_color_output);

  ViewerOutput* viewer_;

  VideoParams video_params_;

  AudioParams audio_params_;

  QVector<RenderTicketWatcher*> running_watchers_;
  std::list<RenderTicketWatcher*> finished_watchers_;
  int running_tickets_;
  QMutex finished_watcher_mutex_;
  QWaitCondition finished_watcher_wait_cond_;

  bool native_progress_signalling_;

  int64_t total_number_of_frames_;
  int64_t total_number_of_unique_frames_;

private slots:
  void TicketDone(RenderTicketWatcher *watcher);

};

}

#endif // RENDERTASK_H
