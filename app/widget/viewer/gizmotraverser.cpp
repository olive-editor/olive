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

#include "gizmotraverser.h"

namespace olive {

QVariant GizmoTraverser::ProcessVideoFootage(const FootageJob &ref, const rational &input_time)
{
  Q_UNUSED(input_time)

  VideoParams stream = ref.video_params();

  return QVector2D(stream.width() * stream.pixel_aspect_ratio().toDouble(),
                   stream.height());
}

QVariant GizmoTraverser::ProcessShader(const Node *node, const TimeRange &range, const ShaderJob &job)
{
  Q_UNUSED(node)
  Q_UNUSED(range)
  Q_UNUSED(job)

  return GenerateResolution();
}

QVariant GizmoTraverser::ProcessFrameGeneration(const Node *node, const GenerateJob &job)
{
  Q_UNUSED(node)
  Q_UNUSED(job)

  return GenerateResolution();
}
}
