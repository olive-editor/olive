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
#include <QMatrix4x4>

#include "project/item/sequence/sequence.h"

OLIVE_NAMESPACE_ENTER

CacheTask::CacheTask(RenderBackend *backend, bool in_out_only) :
  RenderTask(backend),
  in_out_only_(in_out_only)
{
  Init();
}

CacheTask::CacheTask(ViewerOutput* viewer, const VideoParams& vparams, const AudioParams &aparams, bool in_out_only) :
  RenderTask(viewer, vparams, aparams),
  in_out_only_(in_out_only)
{
  Init();
}

bool CacheTask::Run()
{
  // Get list of invalidated ranges
  TimeRangeList video_range = viewer()->video_frame_cache()->GetInvalidatedRanges();
  TimeRangeList audio_range = viewer()->audio_playback_cache()->GetInvalidatedRanges();

  // If we're caching only in-out, limit the range to that
  if (in_out_only_) {
    Sequence* s = static_cast<Sequence*>(viewer()->parent());

    if (s->workarea()->enabled()) {
      video_range = video_range.Intersects(s->workarea()->range());
      audio_range = audio_range.Intersects(s->workarea()->range());
    }
  }

  Render(video_range, audio_range, QMatrix4x4(), true);

  download_threads_.waitForDone();

  return true;
}

QFuture<void> CacheTask::DownloadFrame(FramePtr frame, const QByteArray &hash)
{
  return QtConcurrent::run(&download_threads_, FrameHashCache::SaveCacheFrame, hash, frame);
}

void CacheTask::FrameDownloaded(const QByteArray &hash, const std::list<rational> &times)
{
  foreach (const rational& t, times) {
    viewer()->video_frame_cache()->SetHash(t, hash, job_time());
  }
}

void CacheTask::AudioDownloaded(const TimeRange &range, SampleBufferPtr samples)
{
  if (samples) {
    viewer()->audio_playback_cache()->WritePCM(range, samples, job_time());
  } else {
    viewer()->audio_playback_cache()->WriteSilence(range);
  }
}

void CacheTask::Init()
{
  SetTitle(tr("Caching \"%1\"").arg(viewer()->media_name()));

  backend()->EnablePreviewGeneration(job_time());

  // Render fastest quality
  backend()->SetRenderMode(RenderMode::kOffline);
}

OLIVE_NAMESPACE_EXIT
