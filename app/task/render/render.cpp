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

#include "render.h"

#include "common/timecodefunctions.h"
#include "node/project/sequence/sequence.h"
#include "render/rendermanager.h"

namespace olive {

RenderTask::RenderTask() :
  running_tickets_(0),
  native_progress_signalling_(true)
{
}

RenderTask::~RenderTask()
{
}

bool RenderTask::Render(ColorManager* manager,
                        const TimeRangeList& video_range,
                        const TimeRangeList &audio_range, const TimeRange &subtitle_range,
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

  // Store real time before any rendering takes place
  // Queue audio jobs
  foreach (const TimeRange& range, audio_range) {
    // Don't count audio progress, since it's generally a lot faster than video and is weighted at
    // 50%, which makes the progress bar look weird to the uninitiated
    //total_length += r.length().toDouble();

    rational r = range.in();
    while (r != range.out()) {
      rational end = qMin(range.out(), r+1);
      TimeRange this_range(r, end);

      RenderTicketWatcher* watcher = new RenderTicketWatcher();
      watcher->setProperty("range", QVariant::fromValue(this_range));
      PrepareWatcher(watcher, &watcher_thread);
      IncrementRunningTickets();
      watcher->SetTicket(RenderManager::instance()->RenderAudio(viewer_, this_range, audio_params_, mode, false));

      r = end;
    }
  }

  // Look up hashes
  QMap<QByteArray, QVector<rational> > time_map;
  QVector<QPair<rational, QByteArray> > frame_render_order;

  if (!video_range.isEmpty() && viewer()->GetConnectedTextureOutput()) {
    // Get list of discrete frames from range
    TimeRangeListFrameIterator iterator(video_range, video_params().frame_rate_as_time_base());
    QVector<rational> times(iterator.size());
    QVector<QByteArray> hashes(iterator.size());

    // Generate hashes
    rational r;
    for (int i=0; iterator.GetNext(&r); i++) {
      if (IsCancelled()) {
        return true;
      }

      times[i] = r;
      hashes[i] = RenderManager::instance()->Hash(viewer()->GetConnectedTextureOutput(), viewer()->GetConnectedTextureValueHint(), video_params_, r);
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
    total_number_of_frames_ = times.size();
    total_number_of_unique_frames_ = time_map.size();
    total_length += total_number_of_unique_frames_;
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

  bool result = true;

  // Subtitle loop, loops over all blocks in sequence on all tracks
  if (!subtitle_range.length().isNull()) {
    Sequence *sequence = dynamic_cast<Sequence*>(viewer_);
    if (sequence) {
      TrackList *list = sequence->track_list(Track::kSubtitle);
      QVector<int> block_indexes(list->GetTrackCount(), 0);

      QVector<int> tracks_to_push;
      do {
        tracks_to_push.clear();

        for (int i=0; i<block_indexes.size(); i++) {
          Track *this_track = list->GetTrackAt(i);
          int &this_block_index = block_indexes[i];
          if (this_block_index >= this_track->Blocks().size()) {
            continue;
          }
          Block *this_block = this_track->Blocks().at(this_block_index);

          Track *compare_track = tracks_to_push.isEmpty() ? nullptr : list->GetTrackAt(tracks_to_push.first());
          const int &compare_block_index = tracks_to_push.isEmpty() ? -1 : block_indexes.at(tracks_to_push.first());
          Block *compare_block = compare_track ? compare_track->Blocks().at(compare_block_index) : nullptr;
          if (!compare_track || compare_block->in() >= this_block->in()) {
            if (compare_track && compare_block->in() != this_block->in()) {
              tracks_to_push.clear();
            }
            tracks_to_push.append(i);
          }
        }

        for (int i=0; i<tracks_to_push.size(); i++) {
          Track *this_track = list->GetTrackAt(tracks_to_push.at(i));
          Block *this_block = this_track->Blocks().at(block_indexes.at(tracks_to_push.at(i)));

          if (const SubtitleBlock *sub = dynamic_cast<const SubtitleBlock*>(this_block)) {
            if (!EncodeSubtitle(sub)) {
              result = false;
              break;
            }
          }

          block_indexes[tracks_to_push.at(i)]++;
        }
      } while (!tracks_to_push.isEmpty());
    }
  }

  finished_watcher_mutex_.lock();

  while (result && !IsCancelled()) {
    while (!finished_watchers_.empty() && !IsCancelled() && result) {
      RenderTicketWatcher* watcher = finished_watchers_.front();
      finished_watchers_.pop_front();

      finished_watcher_mutex_.unlock();

      // Analyze watcher here
      RenderManager::TicketType ticket_type = watcher->GetTicket()->property("type").value<RenderManager::TicketType>();

      if (ticket_type == RenderManager::kTypeAudio) {

        TimeRange range = watcher->property("range").value<TimeRange>();

        if (!AudioDownloaded(range, watcher->Get().value<SampleBufferPtr>())) {
          result = false;
        }

        // Don't count audio progress, since it's generally a lot faster than video and is weighted at
        // 50%, which makes the progress bar look weird to the uninitiated
        //progress_counter += range.length().toDouble();
        //emit ProgressChanged(progress_counter / total_length);

      } else if (ticket_type == RenderManager::kTypeVideo && TwoStepFrameRendering()) {

        if (!DownloadFrame(&watcher_thread,
                           watcher->Get().value<FramePtr>(),
                           watcher->property("hash").toByteArray())) {
          result = false;
        }

        if (native_progress_signalling_) {
          progress_counter += 0.5;
          emit ProgressChanged(progress_counter / total_length);
        }

      } else {

        // Assume single-step video or video download ticket
        QByteArray rendered_hash = watcher->property("hash").toByteArray();
        if (!FrameDownloaded(watcher->Get().value<FramePtr>(), rendered_hash, time_map.value(rendered_hash))) {
          result = false;
        }

        if (native_progress_signalling_) {
          double progress_to_add = 1.0;
          if (TwoStepFrameRendering()) {
            progress_to_add *= 0.5;
          }
          progress_counter += progress_to_add;

          emit ProgressChanged(progress_counter / total_length);
        }

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

    if (IsCancelled() || !result) {
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

  if (IsCancelled() || !result) {
    // Cancel every watcher we created
    foreach (RenderTicketWatcher* watcher, running_watchers_) {
      disconnect(watcher, &RenderTicketWatcher::Finished, this, &RenderTask::TicketDone);
      RenderManager::instance()->RemoveTicket(watcher->GetTicket());
    }
  }

  watcher_thread.quit();
  watcher_thread.wait();

  return result;
}

bool RenderTask::DownloadFrame(QThread *thread, FramePtr frame, const QByteArray &hash)
{
  RenderTicketWatcher* watcher = new RenderTicketWatcher();
  watcher->setProperty("hash", hash);
  PrepareWatcher(watcher, thread);

  IncrementRunningTickets();

  watcher->SetTicket(RenderManager::instance()->SaveFrameToCache(viewer_->video_frame_cache(),
                                                                 frame,
                                                                 hash));

  // NOTE: Doesn't reflect the actual return result of SaveFrameToCache
  return true;
}

bool RenderTask::EncodeSubtitle(const SubtitleBlock *subtitle)
{
  Q_UNUSED(subtitle)
  return true;
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
