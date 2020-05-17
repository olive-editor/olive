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

#include "cache.h"

#include <QLinkedList>

#include "common/timecodefunctions.h"
#include "project/item/sequence/sequence.h"
#include "render/backend/opengl/openglbackend.h"

OLIVE_NAMESPACE_ENTER

CacheTask::CacheTask(ViewerOutput* viewer, int divider, bool in_out_only) :
  viewer_(viewer),
  in_out_only_(in_out_only),
  divider_(divider)
{
  SetTitle(tr("Caching \"%1\"").arg(viewer_->media_name()));
}

struct TimeHashFuturePair {
  rational time;
  QFuture<QByteArray> hash_future;
};

struct HashFrameFuturePair {
  QByteArray hash;
  QFuture<FramePtr> frame_future;
};

struct HashDownloadFuturePair {
  QByteArray hash;
  QFuture<void> download_future;
};

struct HashTimePair {
  rational time;
  QByteArray hash;
};

void CacheTask::Action()
{
  OpenGLBackend backend;

  RenderMode::Mode mode = RenderMode::kOffline;
  PixelFormat::Format format = PixelFormat::instance()->GetConfiguredFormatForMode(mode);

  backend.SetViewerNode(viewer_);
  backend.SetPixelFormat(format);
  backend.SetMode(mode);
  backend.SetDivider(divider_);
  backend.SetSampleFormat(SampleFormat::kInternalFormat);

  // Get list of invalidated ranges
  TimeRangeList range_to_cache = viewer_->video_frame_cache()->GetInvalidatedRanges();

  // If we're caching only in-out, limit the range to that
  if (in_out_only_) {
    Sequence* s = static_cast<Sequence*>(viewer_->parent());

    if (s->workarea()->enabled()) {
      range_to_cache = range_to_cache.Intersects(s->workarea()->range());
    }
  }
  // Get hashes for each frame
  QLinkedList<TimeHashFuturePair> hash_list;

  while (!range_to_cache.isEmpty()) {
    const TimeRange& range = range_to_cache.first();

    const rational& timebase = viewer_->video_params().time_base();

    rational time = range.in();
    rational snapped = Timecode::snap_time_to_timebase(time, timebase);
    rational next;

    if (snapped > time) {
      next = snapped;
      snapped -= timebase;
    } else {
      next = snapped + timebase;
    }

    hash_list.append({snapped, backend.Hash(snapped, false)});
    range_to_cache.RemoveTimeRange(TimeRange(snapped, next));
  }

  // Determine any duplicates
  QMap< QByteArray, QLinkedList<rational> > times_to_render;
  foreach (const TimeHashFuturePair& i, hash_list) {
    times_to_render[i.hash_future.result()].append(i.time);
  }

  // Render all frames necessary
  QLinkedList<HashFrameFuturePair> render_lookup_table;
  {
    QLinkedList<HashTimePair> sorted_times;
    QLinkedList<HashTimePair>::iterator sorted_iterator;

    // Rendering is more efficient if we cache in order
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
      render_lookup_table.append({p.hash, backend.RenderFrame(p.time, false, false)});
    }
  }

  OIIO::TypeDesc output_desc = PixelFormat::GetOIIOTypeDesc(format);
  OIIO::ImageSpec output_spec(viewer_->video_params().width() / divider_,
                              viewer_->video_params().height() / divider_,
                              PixelFormat::ChannelCount(format),
                              output_desc);

  // Start downloading frames that have finished
  {
    int counter = 0;
    int nb_frames = render_lookup_table.size();

    QLinkedList<HashDownloadFuturePair> download_futures;

    // Iterators
    QLinkedList<HashFrameFuturePair>::iterator i;
    QLinkedList<HashDownloadFuturePair>::iterator j;

    while (!render_lookup_table.isEmpty() || !download_futures.isEmpty()) {
      i = render_lookup_table.begin();

      while (i != render_lookup_table.end()) {
        if (i->frame_future.isFinished()) {
          FramePtr f = i->frame_future.result();

          // Start multithreaded download here
          download_futures.append({i->hash,
                                  QtConcurrent::run(FrameHashCache::SaveCacheFrame, i->hash, f)});

          i = render_lookup_table.erase(i);
        } else {
          i++;
        }
      }

      j = download_futures.begin();

      while (j != download_futures.end()) {
        if (j->download_future.isFinished()) {
          // Place it in the cache
          const QLinkedList<rational>& times_with_hash = times_to_render.value(j->hash);
          foreach (const rational& t, times_with_hash) {
            viewer_->video_frame_cache()->SetHash(t, j->hash);
          }

          // Signal process
          counter++;
          emit ProgressChanged(qRound(100.0 * static_cast<double>(counter) / static_cast<double>(nb_frames)));

          j = download_futures.erase(j);
        } else {
          j++;
        }
      }
    }
  }
}

OLIVE_NAMESPACE_EXIT
