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

OLIVE_NAMESPACE_ENTER

OpenGLWorker::OpenGLWorker(OpenGLProxy* proxy) :
  proxy_(proxy)
{
}

void OpenGLWorker::TextureToFrame(const QVariant &texture, FramePtr frame, const QMatrix4x4& mat) const
{
  QMetaObject::invokeMethod(proxy_,
                            "TextureToBuffer",
                            Qt::BlockingQueuedConnection,
                            Q_ARG(const QVariant&, texture),
                            OLIVE_NS_ARG(FramePtr, frame),
                            Q_ARG(const QMatrix4x4&, mat));
}

NodeValue OpenGLWorker::FrameToTexture(DecoderPtr decoder, StreamPtr stream, const TimeRange &range) const
{
  FramePtr frame = decoder->RetrieveVideo(range.in(),
                                          video_params().divider(),
                                          video_params().mode() == RenderMode::kOffline);

  NodeValue value;

  if (frame) {
    QMetaObject::invokeMethod(proxy_,
                              "FrameToValue",
                              Qt::BlockingQueuedConnection,
                              OLIVE_NS_RETURN_ARG(NodeValue, value),
                              OLIVE_NS_ARG(FramePtr, frame),
                              OLIVE_NS_ARG(StreamPtr, stream),
                              OLIVE_NS_CONST_ARG(VideoRenderingParams&, video_params()));
  }

  return value;
}

void OpenGLWorker::ProcessNodeEvent(const Node *node, const TimeRange &range, NodeValueDatabase &input_params, NodeValueTable &output_params)
{
  RenderWorker::ProcessNodeEvent(node, range, input_params, output_params);

  if (node->GetCapabilities(input_params) & Node::kShader) {
    QMetaObject::invokeMethod(proxy_,
                              "RunNodeAccelerated",
                              Qt::BlockingQueuedConnection,
                              OLIVE_NS_CONST_ARG(Node*, node),
                              OLIVE_NS_CONST_ARG(TimeRange&, range),
                              OLIVE_NS_ARG(NodeValueDatabase&, input_params),
                              OLIVE_NS_ARG(NodeValueTable&, output_params),
                              OLIVE_NS_CONST_ARG(VideoRenderingParams&, video_params()));
  }
}

OLIVE_NAMESPACE_EXIT
