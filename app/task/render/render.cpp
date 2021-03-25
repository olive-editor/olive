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

#include "render.h"

#include "common/timecodefunctions.h"
#include "render/rendermanager.h"

namespace olive {

RenderTask::RenderTask(Sequence* viewer, const VideoParams &vparams, const AudioParams &aparams) :
  viewer_(viewer),
  video_params_(vparams),
  audio_params_(aparams),
  running_tickets_(0)
{
}

RenderTask::~RenderTask()
{
}

bool RenderTask::Render(ColorManager* manager,
                        const TimeRangeList& video_range,
                        const TimeRangeList &audio_range,
                        RenderMode::Mode mode,
                        FrameHashCache* cache, const QSize &force_size,
                        const QMatrix4x4 &force_matrix, VideoParams::Format force_format,
                        ColorProcessorPtr force_color_output)
{
  // Run watchers in another thread so they can accept signals even while this thread is blocked
  QThread watcher_thread;
  watcher_thread.start();

  double progress_counter = 0;
  double total_length = 0;
  double video_frame_sz = video_params().time_base().toDouble();

  // Store real time before any rendering takes place
  qint64 job_time = QDateTime::currentMSecsSinceEpoch();

  // Queue audio jobs
  foreach (const TimeRange& r, audio_range) {
    // Don't count audio progress, since it's generally a lot faster than video and is weighted at
    // 50%, which makes the progress bar look weird to the uninitiated
    //total_length += r.length().toDouble();

    IncrementRunningTickets();

    RenderTicketWatcher* watcher = new RenderTicketWatcher();
    watcher->setProperty("range", QVariant::fromValue(r));
    PrepareWatcher(watcher, &watcher_thread);
    watcher->SetTicket(RenderManager::instance()->RenderAudio(viewer_, r, audio_params_, false));
  }

  // Look up hashes
  QMap<QByteArray, QVector<rational> > time_map;
  QVector<QPair<rational, QByteArray> > frame_render_order;

  if (!video_range.isEmpty()) {
    // Get list of discrete frames from range
    QVector<rational> times = viewer()->video_frame_cache()->GetFrameListFromTimeRange(video_range);
    QVector<QByteArray> hashes(times.size());

    // Generate hashes
    for (int i=0; i<times.size(); i++) {
      if (IsCancelled()) {
        return true;
      }

      hashes[i] = RenderManager::instance()->Hash(viewer(), video_params_, times.at(i));
    }

    // Filter out duplicates
    for (int i=0; i<hashes.size(); i++) {
      if (IsCancelled()) {
        return true;
      }

      const QByteArray& hash = hashes.at(i);

      QVector<rational>& hash_time_list = time_map[hash];
      hash_time_list.append(times.at(i));

      if (hash_time_list.size() == 1) {
        frame_render_order.append({times.at(i), hash});
      }
    }

    // Add to "total progress"
    total_length += video_frame_sz * time_map.size();
  }

  // Start a render of a limited amount, and then render one frame for each frame that gets
  // finished. This prevents rendered frames from stacking up in memory indefinitely while the
  // encoder is processing them. The amount is kind of arbitrary, but we use the thread count so
  // each of the system's threads are utilized as memory allows.
  const int maximum_rendered_frames = QThread::idealThreadCount();
  auto frame_iterator = frame_render_order.cbegin();

  for (int i=0; i<maximum_rendered_frames && frame_iterator!=frame_render_order.cend(); i++, frame_iterator++) {
    StartTicket(frame_iterator->second, &watcher_thread, manager, frame_iterator->first,
                mode, cache, force_size, force_matrix, force_format, force_color_output);
  }

  finished_watcher_mutex_.lock();

  while (!IsCancelled()) {
    while (!finished_watchers_.empty() && !IsCancelled()) {
      RenderTicketWatcher* watcher = finished_watchers_.front();
      finished_watchers_.pop_front();

      finished_watcher_mutex_.unlock();

      // Analyze watcher here
      RenderManager::TicketType ticket_type = watcher->GetTicket()->property("type").value<RenderManager::TicketType>();

      if (ticket_type == RenderManager::kTypeAudio) {

        TimeRange range = watcher->property("range").value<TimeRange>();

        AudioDownloaded(range,
                        watcher->Get().value<SampleBufferPtr>(),
                        job_time);

        // Don't count audio progress, since it's generally a lot faster than video and is weighted at
        // 50%, which makes the progress bar look weird to the uninitiated
        //progress_counter += range.length().toDouble();
        //emit ProgressChanged(progress_counter / total_length);

      } else if (ticket_type == RenderManager::kTypeVideo && TwoStepFrameRendering()) {

        DownloadFrame(&watcher_thread,
                      watcher->Get().value<FramePtr>(),
                      watcher->property("hash").toByteArray());

        progress_counter += video_frame_sz * 0.5;
        emit ProgressChanged(progress_counter / total_length);

      } else {

        // Assume single-step video or video download ticket
        QByteArray rendered_hash = watcher->property("hash").toByteArray();
        FrameDownloaded(watcher->Get().value<FramePtr>(), rendered_hash, time_map.value(rendered_hash), job_time);

        double progress_to_add = video_frame_sz;
        if (TwoStepFrameRendering()) {
          progress_to_add *= 0.5;
        }
        progress_counter += progress_to_add;

        emit ProgressChanged(progress_counter / total_length);

        if (frame_iterator != frame_render_order.cend()) {
          StartTicket(frame_iterator->second, &watcher_thread, manager, frame_iterator->first,
                      mode, cache, force_size, force_matrix, force_format, force_color_output);

          frame_iterator++;
        }

      }

      delete watcher;
      running_watchers_.removeOne(watcher);

      finished_watcher_mutex_.lock();
    }

    if (IsCancelled()) {
      break;
    }

    // Run out of finished watchers. If we still have running tickets, wait for the next one to finish.
    if (running_tickets_ > 0) {
      finished_watcher_wait_cond_.wait(&finished_watcher_mutex_);
    } else {
      // No more running tickets or finished tickets, wem ust be
      break;
    }
  }

  finished_watcher_mutex_.unlock();

  if (IsCancelled()) {
    // Cancel every watcher we created
    foreach (RenderTicketWatcher* watcher, running_watchers_) {
      disconnect(watcher, &RenderTicketWatcher::Finished, this, &RenderTask::TicketDone);
      watcher->Cancel();
    }
  }

  watcher_thread.quit();
  watcher_thread.wait();

  return true;
}

void RenderTask::DownloadFrame(QThread *thread, FramePtr frame, const QByteArray &hash)
{
  RenderTicketWatcher* watcher = new RenderTicketWatcher();
  watcher->setProperty("hash", hash);
  PrepareWatcher(watcher, thread);

  IncrementRunningTickets();

  watcher->SetTicket(RenderManager::instance()->SaveFrameToCache(viewer_->video_frame_cache(),
                                                                 frame,
                                                                 hash));
}

void RenderTask::PrepareWatcher(RenderTicketWatcher *watcher, QThread *thread)
{
  watcher->moveToThread(thread);
  connect(watcher, &RenderTicketWatcher::Finished, this, &RenderTask::TicketDone, Qt::DirectConnection);
  running_watchers_.append(watcher);
}

void RenderTask::IncrementRunningTickets()
{
  finished_watcher_mutex_.lock();
  running_tickets_++;
  finished_watcher_mutex_.unlock();
}

void RenderTask::StartTicket(const QByteArray& hash, QThread* watcher_thread, ColorManager* manager,
                             const rational& time, RenderMode::Mode mode, FrameHashCache* cache,
                             const QSize &force_size, const QMatrix4x4 &force_matrix,
                             VideoParams::Format force_format, ColorProcessorPtr force_color_output)
{
  RenderTicketWatcher* watcher = new RenderTicketWatcher();
  watcher->setProperty("hash", hash);
  PrepareWatcher(watcher, watcher_thread);

  IncrementRunningTickets();

  watcher->SetTicket(RenderManager::instance()->RenderFrame(viewer_, manager, time,
                                                            mode, video_params_, audio_params_,
                                                            force_size, force_matrix,
                                                            force_format, force_color_output,
                                                            cache));
}

void RenderTask::TicketDone(RenderTicketWatcher* watcher)
{
  finished_watcher_mutex_.lock();
  finished_watchers_.push_back(watcher);
  finished_watcher_wait_cond_.wakeAll();
  running_tickets_--;
  finished_watcher_mutex_.unlock();
}

}
