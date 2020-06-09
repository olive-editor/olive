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

void GizmoTraverser::ProcessNodeEvent(const Node *node, const TimeRange &range, NodeValueDatabase &input_params_in, NodeValueTable &output_params)
{
  // Convert footage to image/sample buffers
  QList<NodeInput*> inputs = node->GetInputsIncludingArrays();
  foreach (NodeInput* input, inputs) {
    if (input->data_type() == NodeParam::kFootage) {
      StreamPtr stream = ResolveStreamFromInput(input);

      if (stream
          && (stream->type() == Stream::kVideo || stream->type() == Stream::kImage)) {
        ImageStreamPtr image_stream = std::static_pointer_cast<ImageStream>(stream);

        output_params.Push(NodeParam::kTexture,
                           QSize(image_stream->width(), image_stream->height()),
                           node);
      } else if (stream->type() == Stream::kAudio) {
        // FIXME: Do something...
      }
    }
  }
}

OLIVE_NAMESPACE_EXIT
