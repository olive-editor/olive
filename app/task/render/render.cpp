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

RenderTask::RenderTask(ViewerOutput* viewer, const VideoRenderingParams &vparams, const AudioRenderingParams &aparams) :
  viewer_(viewer),
  video_params_(vparams),
  audio_params_(aparams)
{
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

  std::list<RangeSampleFuturePair> audio_lookup_table;
  if (!audio_range.isEmpty()) {
    foreach (const TimeRange& r, audio_range) {
      QList<TimeRange> ranges = RenderBackend::SplitRangeIntoChunks(r);

      foreach (const TimeRange& split, ranges) {
        audio_lookup_table.push_back({split, backend_.RenderAudio(split)});
      }
    }
  }

  // Get hashes for each frame and group likes together
  int progress_counter = 0;
  int nb_frames;

  QMap<QByteArray, rational> times_to_render;

  std::list<HashFrameFuturePair> render_lookup_table;
  QList<rational> times;
  QList<QByteArray> hashes;

  if (!video_range.isEmpty()) {

    {
      QList<QByteArray> existing_hashes;

      times = viewer_->video_frame_cache()->GetFrameListFromTimeRange(video_range);
      nb_frames = times.size();

      QFuture<QList<QByteArray> > hash_future = backend_.Hash(times);
      hashes = hash_future.result();

      // Determine any duplicates
      for (int index=0;index<hashes.size();index++) {
        const QByteArray& hash = hashes.at(index);
        const rational& time = times.at(index);

        bool map_contains_hash = times_to_render.contains(hash);

        bool hash_exists = false;

        if (use_disk_cache && !map_contains_hash) {
          hash_exists = existing_hashes.contains(hash);

          if (!hash_exists) {
            hash_exists = QFileInfo::exists(viewer_->video_frame_cache()->CachePathName(hash, video_params_.format()));

            if (hash_exists) {
              existing_hashes.append(hash);
            }
          }

          if (hash_exists) {
            // Already exists, no need to render it again
            FrameDownloaded(hash, {time});
            progress_counter++;
            emit ProgressChanged(static_cast<double>(progress_counter) / static_cast<double>(nb_frames));
          }
        }

        if (!hash_exists && (!map_contains_hash || times_to_render.value(hash) > time)) {
          times_to_render.insert(hash, time);
        }
      }
    }

    // Render all frames necessary
    {
      QMap<QByteArray, rational>::const_iterator i;
      std::list<HashTimePair>::iterator j;

      std::list<HashTimePair> sorted_times;

      // Rendering is generally more efficient when done sequentially, so we sort the
      // times chronologically here
      for (i=times_to_render.begin(); i!=times_to_render.end(); i++) {
        bool inserted = false;

        for (j=sorted_times.begin(); j!=sorted_times.end(); j++) {
          if (j->time > i.value()) {
            sorted_times.insert(j, {i.value(), i.key()});\
            inserted = true;
            break;
          }
        }

        if (!inserted) {
          sorted_times.push_back({i.value(), i.key()});
        }
      }

      // Start render jobs in sorted order
      for (j=sorted_times.begin(); j!=sorted_times.end(); j++) {
        render_lookup_table.push_back({j->hash, backend_.RenderFrame(j->time)});
      }
    }
  }

  // Start downloading frames that have finished
  std::list<HashDownloadFuturePair> download_futures;

  // Iterators
  std::list<HashFrameFuturePair>::iterator i;
  std::list<HashDownloadFuturePair>::iterator j;
  std::list<RangeSampleFuturePair>::iterator k;

  while (!IsCancelled()
         && (!render_lookup_table.empty()
             || !download_futures.empty()
             || !audio_lookup_table.empty())) {

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

        // Signal process
        progress_counter += times_with_hash.size();
        emit ProgressChanged(static_cast<double>(progress_counter) / static_cast<double>(nb_frames));

        j = download_futures.erase(j);

      } else {
        j++;
      }
    }

    k = audio_lookup_table.begin();

    while (!IsCancelled() && k != audio_lookup_table.end()) {
      if (k->sample_future->IsFinished()) {
        AudioDownloaded(k->range, k->sample_future->Get().value<SampleBufferPtr>());

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
