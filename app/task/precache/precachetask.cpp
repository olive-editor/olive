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

PreCacheTask::PreCacheTask(Footage *footage, int index, Sequence* sequence)
{
  // Set video and audio params
  set_video_params(sequence->GetVideoParams());
  set_audio_params(sequence->GetAudioParams());

  // Create new project
  project_ = new Project();

  // Create viewer with same parameters as the sequence
  set_viewer(new ViewerOutput());
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

  Node::ConnectEdge(footage_, NodeInput(viewer(), ViewerOutput::kTextureInput));
  viewer()->SetValueHintForInput(ViewerOutput::kTextureInput, Node::ValueHint({NodeValue::kTexture}, Track::Reference(Track::kVideo, index).ToString()));

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
  TimeRange intersection;

  if (footage_->GetTimelinePoints()->workarea()->enabled()) {
    // If we're caching only in-out, limit the range to that
    intersection = footage_->GetTimelinePoints()->workarea()->range();
  } else {
    // Otherwise use full length
    intersection = TimeRange(0, footage_->GetVideoLength());
  }

  TimeRangeList video_range = viewer()->video_frame_cache()->GetInvalidatedRanges(intersection);

  Render(project_->color_manager(),
         video_range,
         TimeRangeList(),
         TimeRange(),
         RenderMode::kOnline,
         viewer()->video_frame_cache());

  return true;
}

bool PreCacheTask::FrameDownloaded(FramePtr frame, const QByteArray &hash, const QVector<rational> &times)
{
  // Do nothing. Pre-cache essentially just creates more frames in the cache, it doesn't need to do
  // anything else.

  Q_UNUSED(frame)
  Q_UNUSED(hash)
  Q_UNUSED(times)

  return true;
}

bool PreCacheTask::AudioDownloaded(const TimeRange &range, SampleBufferPtr samples)
{
  // Pre-cache doesn't cache any audio

  Q_UNUSED(range)
  Q_UNUSED(samples)

  return true;
}

}
