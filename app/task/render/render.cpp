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
#include "render/backend/opengl/openglbackend.h"

OLIVE_NAMESPACE_ENTER

RenderTask::RenderTask(ViewerOutput* viewer, const VideoRenderingParams &vparams, const AudioRenderingParams &aparams) :
  viewer_(viewer),
  video_params_(vparams),
  audio_params_(aparams)
{
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

void RenderTask::Render(const TimeRangeList& range_to_cache,
                        const QMatrix4x4& mat,
                        bool audio_enabled,
                        bool use_disk_cache)
{
  OpenGLBackend backend;

  backend.moveToThread(qApp->thread());

  backend.SetViewerNode(viewer_);
  backend.SetVideoParams(video_params_);
  backend.SetAudioParams(audio_params_);
  backend.SetVideoDownloadMatrix(mat);

  // Get hashes for each frame and group likes together
  QMap< QByteArray, QLinkedList<rational> > times_to_render;

  {
    QList<rational> times = viewer_->video_frame_cache()->GetFrameListFromTimeRange(range_to_cache);

    QFuture<QList<QByteArray> > hash_future = backend.Hash(times);
    QList<QByteArray> hashes = hash_future.result();

    // Determine any duplicates
    int index = 0;
    foreach (const QByteArray& hash, hashes) {
      const rational& time = times.at(index);

      if (use_disk_cache
          && QFileInfo::exists(viewer_->video_frame_cache()->CachePathName(hash, video_params_.format()))) {
        // Already exists, no need to render it again
        FrameDownloaded(hash, {time});
      } else {
        times_to_render[hash].append(time);
      }

      index++;
    }
  }

  // Render all frames necessary
  QLinkedList<HashFrameFuturePair> render_lookup_table;
  {
    QLinkedList<HashTimePair> sorted_times;
    QLinkedList<HashTimePair>::iterator sorted_iterator;

    // Rendering is more efficient if we cache in order, so here we sort
    QMap< QByteArray, QLinkedList<rational> >::const_iterator i;
    for (i=times_to_render.constBegin(); i!=times_to_render.constEnd(); i++) {
      const QByteArray& hash = i.key();
      const rational& time = i.value().first();

      bool inserted = false;

      for (sorted_iterator=sorted_times.begin(); sorted_iterator!=sorted_times.end(); sorted_iterator++) {
        if (sorted_iterator->time > time) {
          sorted_times.insert(sorted_iterator, {time, hash});
          inserted = true;
          break;
        }
      }

      if (!inserted) {
        sorted_times.append({time, hash});
      }
    }

    foreach (const HashTimePair& p, sorted_times) {
      render_lookup_table.append({p.hash, backend.RenderFrame(p.time)});
    }
  }

  QLinkedList<RangeSampleFuturePair> audio_lookup_table;
  if (audio_enabled) {
    foreach (const TimeRange& r, range_to_cache) {
      QList<TimeRange> ranges = RenderBackend::SplitRangeIntoChunks(r);

      foreach (const TimeRange& split, ranges) {
        audio_lookup_table.append({split, backend.RenderAudio(split)});
      }
    }
  }

  // Start downloading frames that have finished

  int counter = 0;
  int nb_frames = render_lookup_table.size();

  QLinkedList<HashDownloadFuturePair> download_futures;

  // Iterators
  QLinkedList<HashFrameFuturePair>::iterator i;
  QLinkedList<HashDownloadFuturePair>::iterator j;
  QLinkedList<RangeSampleFuturePair>::iterator k;

  while (!IsCancelled()
         && (!render_lookup_table.isEmpty()
             || !download_futures.isEmpty()
             || !audio_lookup_table.isEmpty())) {

    i = render_lookup_table.begin();

    while (i != render_lookup_table.end()) {
      if (i->frame_future->IsFinished()) {
        FramePtr f = i->frame_future->Get().value<FramePtr>();

        // Start multithreaded download here
        download_futures.append({i->hash, DownloadFrame(f, i->hash)});

        i = render_lookup_table.erase(i);
      } else {
        i++;
      }
    }

    j = download_futures.begin();

    while (j != download_futures.end()) {
      if (j->download_future.isFinished()) {
        // Place it in the cache
        FrameDownloaded(j->hash, times_to_render.value(j->hash));

        // Signal process
        counter++;
        emit ProgressChanged(static_cast<double>(counter) / static_cast<double>(nb_frames));

        j = download_futures.erase(j);
      } else {
        j++;
      }
    }

    if (audio_enabled) {
      k = audio_lookup_table.begin();

      while (k != audio_lookup_table.end()) {
        if (k->sample_future->IsFinished()) {
          AudioDownloaded(k->range, k->sample_future->Get().value<SampleBufferPtr>());

          k = audio_lookup_table.erase(k);
        } else {
          k++;
        }
      }
    }
  }
}

void RenderTask::AudioDownloaded(const TimeRange &, SampleBufferPtr)
{
}

void RenderTask::SetAnchorPoint(const rational &r)
{
  anchor_point_ = r;
}

OLIVE_NAMESPACE_EXIT
