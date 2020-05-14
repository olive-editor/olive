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

#include "gizmotraverser.h"

OLIVE_NAMESPACE_ENTER

void GizmoTraverser::FootageProcessingEvent(StreamPtr stream, const TimeRange &/*input_time*/, NodeValueTable *table) const
{
  if (stream->type() == Stream::kVideo || stream->type() == Stream::kAudio) {

    ImageStreamPtr image_stream = std::static_pointer_cast<ImageStream>(stream);

    table->Push(NodeParam::kTexture, QSize(image_stream->width(),
                                           image_stream->height()));

  } else if (stream->type() == Stream::kAudio) {
    // FIXME: Get samples
  }
}

OLIVE_NAMESPACE_EXIT
