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

#include "openglworker.h"

#include "common/clamp.h"
#include "core.h"
#include "node/block/transition/transition.h"
#include "node/node.h"
#include "openglcolorprocessor.h"
#include "openglrenderfunctions.h"
#include "render/colormanager.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

OpenGLWorker::OpenGLWorker(VideoRenderFrameCache *frame_cache, QObject *parent) :
  VideoRenderWorker(frame_cache, parent)
{
}

void OpenGLWorker::FrameToValue(DecoderPtr decoder, StreamPtr stream, const TimeRange &range, NodeValueTable *table)
{
  FramePtr frame = decoder->RetrieveVideo(range.in(), video_params().divider());

  if (frame) {
    emit RequestFrameToValue(frame, stream, table);
  }
}

void OpenGLWorker::RunNodeAccelerated(const Node *node, const TimeRange &range, NodeValueDatabase &input_params, NodeValueTable &output_params)
{
  emit RequestRunNodeAccelerated(node, range, input_params, output_params);
}

void OpenGLWorker::TextureToBuffer(const QVariant &tex_in, void *buffer, int linesize)
{
  emit RequestTextureToBuffer(tex_in, buffer, linesize);
}

OLIVE_NAMESPACE_EXIT
