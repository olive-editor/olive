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

#include "precachetask.h"

#include "node/project/project.h"

namespace olive {

PreCacheTask::PreCacheTask(Footage *footage, int index, Sequence* sequence) :
  RenderTask(new ViewerOutput(), sequence->GetVideoParams(), sequence->GetAudioParams())
{
  // Create new project
  project_ = new Project();

  // Create viewer with same parameters as the sequence
  viewer()->setParent(project_);
  viewer()->SetVideoParams(sequence->GetVideoParams());
  viewer()->SetAudioParams(sequence->GetAudioParams());

  // Copy project config nodes
  Node::CopyInputs(footage->project()->color_manager(), project_->color_manager(), false);
  Node::CopyInputs(footage->project()->settings(), project_->settings(), false);

  // Copy footage node so it can precache without any modifications from the user screwing it up
  footage_ = static_cast<Footage*>(footage->copy());
  footage_->setParent(project_);
  Node::CopyInputs(footage, footage_, false);
  Node::ConnectEdge(NodeOutput(footage_, Track::Reference(Track::kVideo, index).ToString()), NodeInput(viewer(), ViewerOutput::kTextureInput));

  SetTitle(tr("Pre-caching %1:%2").arg(footage_->filename(), QString::number(index)));
}

PreCacheTask::~PreCacheTask()
{
  // This should delete the footage we copied and the viewer we created
  delete project_;
}

bool PreCacheTask::Run()
{
  // Get list of invalidated ranges
  TimeRangeList video_range = viewer()->video_frame_cache()->GetInvalidatedRanges();

  // If we're caching only in-out, limit the range to that
  if (footage_->GetTimelinePoints()->workarea()->enabled()) {
    video_range = video_range.Intersects(footage_->GetTimelinePoints()->workarea()->range());
  }

  Render(project_->color_manager(),
         video_range,
         TimeRangeList(),
         TimeRange(),
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
