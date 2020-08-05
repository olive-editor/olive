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

#include "precachetask.h"

OLIVE_NAMESPACE_ENTER

PreCacheTask::PreCacheTask(VideoStreamPtr footage, Sequence* sequence) :
  RenderTask(new ViewerOutput(), sequence->video_params(), sequence->audio_params()),
  footage_(footage)
{
  viewer()->set_video_params(sequence->video_params());
  viewer()->set_audio_params(sequence->audio_params());

  // Render fastest quality
  backend()->SetRenderMode(RenderMode::kOffline);

  video_node_ = new VideoInput();
  video_node_->SetFootage(footage);

  NodeParam::ConnectEdge(video_node_->output(), viewer()->texture_input());

  SetTitle(tr("Pre-caching %1:%2").arg(footage->footage()->filename(),
                                       QString::number(footage->index())));

  backend()->NodeGraphChanged(viewer()->texture_input());
  backend()->ProcessUpdateQueue();
}

PreCacheTask::~PreCacheTask()
{
  delete viewer();
  delete video_node_;
}

bool PreCacheTask::Run()
{
  // Get list of invalidated ranges
  TimeRangeList video_range = viewer()->video_frame_cache()->GetInvalidatedRanges();

  // If we're caching only in-out, limit the range to that
  /*
  if (in_out_only_) {
    Sequence* s = static_cast<Sequence*>(viewer()->parent());

    if (s->workarea()->enabled()) {
      video_range = video_range.Intersects(s->workarea()->range());
    }
  }
  */

  Render(video_range, TimeRangeList(), true);

  download_threads_.waitForDone();

  return true;
}

QFuture<void> PreCacheTask::DownloadFrame(FramePtr frame, const QByteArray &hash)
{
  return QtConcurrent::run(&download_threads_, FrameHashCache::SaveCacheFrame, hash, frame);
}

void PreCacheTask::FrameDownloaded(const QByteArray &hash, const std::list<rational> &times, qint64 job_time)
{
  foreach (const rational& t, times) {
    viewer()->video_frame_cache()->SetHash(t, hash, job_time, true);
  }
}

void PreCacheTask::AudioDownloaded(const TimeRange &range, SampleBufferPtr samples, qint64 job_time)
{
  // Pre-cache doesn't cache any audio

  Q_UNUSED(range)
  Q_UNUSED(samples)
  Q_UNUSED(job_time)
}

OLIVE_NAMESPACE_EXIT
