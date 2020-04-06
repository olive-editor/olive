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

#ifndef OPENGLPROCESSOR_H
#define OPENGLPROCESSOR_H

#include <QOffscreenSurface>
#include <QOpenGLContext>

#include "../videorenderworker.h"
#include "openglframebuffer.h"
#include "openglshadercache.h"
#include "opengltexturecache.h"

OLIVE_NAMESPACE_ENTER

class OpenGLWorker : public VideoRenderWorker {
  Q_OBJECT
public:
  OpenGLWorker(VideoRenderFrameCache* frame_cache, DecoderCache *decoder_cache,
               QObject* parent = nullptr);

signals:
  void RequestFrameToValue(DecoderPtr decoder, StreamPtr stream, const TimeRange &range, NodeValueTable* table);

  void RequestRunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable* output_params);

  void RequestTextureToBuffer(const QVariant& texture, void *buffer);

protected:
  virtual void FrameToValue(DecoderPtr decoder, StreamPtr stream, const TimeRange &range, NodeValueTable* table) override;

  virtual void RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable* output_params) override;

  virtual void TextureToBuffer(const QVariant& texture, void *buffer) override;

};

OLIVE_NAMESPACE_EXIT

#endif // OPENGLPROCESSOR_H
