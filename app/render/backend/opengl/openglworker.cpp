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

OpenGLWorker::OpenGLWorker(RenderBackend *parent) :
  RenderWorker(parent)
{
}

void OpenGLWorker::TextureToFrame(const QVariant &texture, FramePtr frame, const QMatrix4x4& mat) const
{
  QMetaObject::invokeMethod(OpenGLProxy::instance(),
                            "TextureToBuffer",
                            Qt::BlockingQueuedConnection,
                            Q_ARG(const QVariant&, texture),
                            OLIVE_NS_ARG(FramePtr, frame),
                            Q_ARG(const QMatrix4x4&, mat));
}

QVariant OpenGLWorker::FootageFrameToTexture(StreamPtr stream, FramePtr frame) const
{
  QVariant value;

  QMetaObject::invokeMethod(OpenGLProxy::instance(),
                            "FrameToValue",
                            Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(QVariant, value),
                            OLIVE_NS_ARG(FramePtr, frame),
                            OLIVE_NS_ARG(StreamPtr, stream),
                            OLIVE_NS_CONST_ARG(VideoParams&, video_params()),
                            OLIVE_NS_CONST_ARG(RenderMode::Mode&, render_mode()));

  return value;
}

QVariant OpenGLWorker::CachedFrameToTexture(FramePtr frame) const
{
  QVariant value;

  QMetaObject::invokeMethod(OpenGLProxy::instance(),
                            "PreCachedFrameToValue",
                            Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(QVariant, value),
                            OLIVE_NS_ARG(FramePtr, frame));

  return value;
}

QVariant OpenGLWorker::ProcessShader(const Node *node, const TimeRange &range, const ShaderJob &job)
{
  QVariant value;

  QMetaObject::invokeMethod(OpenGLProxy::instance(),
                            "RunNodeAccelerated",
                            Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(QVariant, value),
                            OLIVE_NS_CONST_ARG(Node*, node),
                            OLIVE_NS_CONST_ARG(TimeRange&, range),
                            OLIVE_NS_CONST_ARG(ShaderJob&, job),
                            OLIVE_NS_CONST_ARG(VideoParams&, video_params()));

  return value;
}

bool OpenGLWorker::TextureHasAlpha(const QVariant &v) const
{
  return PixelFormat::FormatHasAlphaChannel(v.value<OpenGLTextureCache::ReferencePtr>()->texture()->format());
}

OLIVE_NAMESPACE_EXIT
