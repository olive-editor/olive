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

QVariant GizmoTraverser::ProcessVideoFootage(StreamPtr stream, const rational &input_time)
{
  Q_UNUSED(input_time)

  ImageStreamPtr image_stream = std::static_pointer_cast<ImageStream>(stream);

  return QSize(image_stream->width(), image_stream->height());
}

QVariant GizmoTraverser::ProcessShader(const Node *node, const TimeRange &range, const ShaderJob &job)
{
  Q_UNUSED(node)
  Q_UNUSED(range)
  Q_UNUSED(job)

  return size_;
}

OLIVE_NAMESPACE_EXIT
