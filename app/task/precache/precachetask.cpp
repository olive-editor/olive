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

#include "precachetask.h"

#include "project/project.h"

namespace olive {

PreCacheTask::PreCacheTask(Footage *footage, int index, Sequence* sequence) :
  RenderTask(new ViewerOutput(), sequence->video_params(), sequence->audio_params())
{
  viewer()->set_video_params(sequence->video_params());
  viewer()->set_audio_params(sequence->audio_params());

  // FIXME: I've been lazy and haven't included support for anything connected to a footage input.
  //        At the moment, footage nodes have no connectable inputs so it's not a problem, but if
  //        they ever do, that needs to be addressed immediately.
  Q_ASSERT(footage->inputs().isEmpty());

  // Copy footage node so it can precache without any modifications from the user screwing it up
  footage_ = static_cast<Footage*>(footage->copy());
  index_ = index;
  Node::CopyInputs(footage, footage_, false);

  Node::ConnectEdge(footage_, NodeInput(viewer(), ViewerOutput::kTextureInput));

  SetTitle(tr("Pre-caching %1:%2").arg(footage_->filename()));
}

PreCacheTask::~PreCacheTask()
{
  // We created this viewer node ourselves, so now we should delete it
  delete viewer();
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

  Render(footage_->project()->color_manager(),
         video_range,
         TimeRangeList(),
         RenderMode::kOnline,
         viewer()->video_frame_cache());

  return true;
}

void PreCacheTask::FrameDownloaded(FramePtr frame, const QByteArray &hash, const QVector<rational> &times, qint64 job_time)
{
  // Do nothing. Pre-cache essentially just creates more frames in the cache, it doesn't need to do
  // anything else.

  Q_UNUSED(frame)
  Q_UNUSED(hash)
  Q_UNUSED(times)
  Q_UNUSED(job_time)
}

void PreCacheTask::AudioDownloaded(const TimeRange &range, SampleBufferPtr samples, qint64 job_time)
{
  // Pre-cache doesn't cache any audio

  Q_UNUSED(range)
  Q_UNUSED(samples)
  Q_UNUSED(job_time)
}

}
