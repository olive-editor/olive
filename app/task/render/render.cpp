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

#include "render.h"

#include "common/timecodefunctions.h"

OLIVE_NAMESPACE_ENTER

RenderTask::RenderTask(ViewerOutput* viewer, const VideoParams &vparams, const AudioParams &aparams) :
  viewer_(viewer),
  video_params_(vparams),
  audio_params_(aparams)
{
  job_time_ = QDateTime::currentMSecsSinceEpoch();

  // FIXME: This makes a full copy of the node graph every time it starts, there must be a better
  //        way.
  backend_.SetViewerNode(viewer_);
  backend_.SetVideoParams(video_params_);
  backend_.SetAudioParams(audio_params_);
}

struct TimeHashFuturePair {
  rational time;
  RenderTicketPtr hash_future;
};

struct HashTimePair {
  rational time;
  QByteArray hash;
};

struct HashFrameFuturePair {
  QByteArray hash;
  RenderTicketPtr frame_future;
};

struct RangeSampleFuturePair {
  TimeRange range;
  RenderTicketPtr sample_future;
};

struct HashDownloadFuturePair {
  QByteArray hash;
  QFuture<void> download_future;
};

void RenderTask::Render(const TimeRangeList& video_range,
                        const TimeRangeList &audio_range,
                        const QMatrix4x4& mat,
                        bool use_disk_cache)
{
  backend_.SetVideoDownloadMatrix(mat);

  double progress_counter = 0;
  double total_length = 0;
  double video_frame_sz = video_params_.time_base().toDouble();

  std::list<RangeSampleFuturePair> audio_lookup_table;
  if (!audio_range.isEmpty()) {
    foreach (const TimeRange& r, audio_range) {
      total_length += r.length().toDouble();

      QList<TimeRange> ranges = RenderBackend::SplitRangeIntoChunks(r);

      foreach (const TimeRange& split, ranges) {
        audio_lookup_table.push_back({split, backend_.RenderAudio(split)});
      }
    }
  }

  std::list<HashFrameFuturePair> render_lookup_table;
  QVector<rational> times;
  QVector<QByteArray> hashes;
  std::list<HashTimePair> frame_queue;

  if (!video_range.isEmpty()) {
    QList<QByteArray> existing_hashes;

    foreach (const TimeRange& r, video_range) {
      total_length += r.length().toDouble();
    }

    times = viewer_->video_frame_cache()->GetFrameListFromTimeRange(video_range);

    QFuture<QVector<QByteArray> > hash_future = backend_.Hash(times);
    hashes = hash_future.result();

    for (int i=0;i<times.size();i++) {
      frame_queue.push_back({times.at(i), hashes.at(i)});
    }
  }

  // Start downloading frames that have finished
  std::list<HashDownloadFuturePair> download_futures;

  // Iterators
  std::list<HashFrameFuturePair>::iterator i;
  std::list<HashDownloadFuturePair>::iterator j;
  std::list<RangeSampleFuturePair>::iterator k;

  std::list<QByteArray> running_hashes;
  std::list<QByteArray> existing_hashes;

  while (!IsCancelled()
         && (!render_lookup_table.empty()
             || !frame_queue.empty()
             || !download_futures.empty()
             || !audio_lookup_table.empty())) {

    if (!frame_queue.empty()) {
      // Pop another frame off the frame queue
      const HashTimePair& p = frame_queue.front();

      // Check if we're already rendering this hash
      bool rendering_hash = (std::find(running_hashes.begin(), running_hashes.end(), p.hash) != running_hashes.end());

      // Skip this hash if we're already rendering it
      if (!rendering_hash) {
        // Check if this frame already exists (has already been rendered previously or during this job)
        bool hash_exists = false;

        if (use_disk_cache) {
          bool hash_exists = (std::find(existing_hashes.begin(), existing_hashes.end(), p.hash) != existing_hashes.end());

          if (!hash_exists) {
            hash_exists = QFileInfo::exists(viewer_->video_frame_cache()->CachePathName(p.hash));

            if (hash_exists) {
              existing_hashes.push_back(p.hash);
            }
          }

          if (hash_exists) {
            // Already exists, no need to render it again
            FrameDownloaded(p.hash, {p.time});
            progress_counter += video_frame_sz;
            emit ProgressChanged(progress_counter / total_length);
          }
        }

        // If no existing disk cache was found, queue it now
        if (!hash_exists) {
          render_lookup_table.push_back({p.hash, backend_.RenderFrame(p.time)});
          running_hashes.push_back(p.hash);
        }
      }

      // Remove first element
      frame_queue.pop_front();
    }

    i = render_lookup_table.begin();

    while (!IsCancelled() && i != render_lookup_table.end()) {
      if (i->frame_future->IsFinished()) {
        FramePtr f = i->frame_future->Get().value<FramePtr>();

        // Start multithreaded download here
        download_futures.push_back({i->hash, DownloadFrame(f, i->hash)});

        i = render_lookup_table.erase(i);
      } else {
        i++;
      }
    }

    j = download_futures.begin();

    while (!IsCancelled() && j != download_futures.end()) {
      if (j->download_future.isFinished()) {
        // Place it in the cache
        std::list<rational> times_with_hash;

        for (int k=0;k<hashes.size();k++) {
          if (hashes.at(k) == j->hash) {
            times_with_hash.push_back(times.at(k));
          }
        }

        FrameDownloaded(j->hash, times_with_hash);

        existing_hashes.push_back(j->hash);

        // Signal process
        progress_counter += times_with_hash.size() * video_frame_sz;
        emit ProgressChanged(progress_counter / total_length);

        j = download_futures.erase(j);

      } else {
        j++;
      }
    }

    k = audio_lookup_table.begin();

    while (!IsCancelled() && k != audio_lookup_table.end()) {
      if (k->sample_future->IsFinished()) {
        AudioDownloaded(k->range, k->sample_future->Get().value<SampleBufferPtr>());

        progress_counter += k->range.length().toDouble();
        emit ProgressChanged(progress_counter / total_length);

        k = audio_lookup_table.erase(k);
      } else {
        k++;
      }
    }
  }

  // `Close` will block until all jobs are done making a safe deletion
  backend_.Close();
}

void RenderTask::SetAnchorPoint(const rational &r)
{
  anchor_point_ = r;
}

OLIVE_NAMESPACE_EXIT
