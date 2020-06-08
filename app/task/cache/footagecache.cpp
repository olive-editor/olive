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

#include "footagecache.h"

#include "common/timecodefunctions.h"

OLIVE_NAMESPACE_ENTER

FootageCacheTask::FootageCacheTask(VideoStreamPtr footage, Sequence *sequence) :
  CacheTask(new ViewerOutput(), sequence->video_params(), sequence->audio_params(), false),
  footage_(footage)
{
  viewer()->set_video_params(sequence->video_params());
  viewer()->set_audio_params(sequence->audio_params());
  backend()->SetVideoParams(sequence->video_params());
  backend()->SetAudioParams(sequence->audio_params());

  video_node_ = new VideoInput();
  video_node_->SetFootage(footage);

  NodeParam::ConnectEdge(video_node_->output(), viewer()->texture_input());

  SetTitle(tr("Pre-caching %1:%2").arg(footage->footage()->filename(),
                                       QString::number(footage->index())));

  backend()->ProcessUpdateQueue();
}

FootageCacheTask::~FootageCacheTask()
{
  delete viewer();
  delete video_node_;
}

OLIVE_NAMESPACE_EXIT
