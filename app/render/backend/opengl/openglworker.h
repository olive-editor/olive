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

#ifndef OPENGLWORKER_H
#define OPENGLWORKER_H

#include "openglproxy.h"
#include "render/backend/renderworker.h"

OLIVE_NAMESPACE_ENTER

class OpenGLWorker : public RenderWorker
{
public:
  OpenGLWorker(RenderBackend* parent);

protected:
  virtual void TextureToFrame(const QVariant& texture, FramePtr frame, const QMatrix4x4 &mat) const override;

  virtual QVariant FootageFrameToTexture(StreamPtr stream, FramePtr frame) const override;

  virtual QVariant CachedFrameToTexture(FramePtr frame) const override;

  virtual QVariant ProcessShader(const Node *node, const TimeRange &range, const ShaderJob& job) override;

  virtual bool TextureHasAlpha(const QVariant& v) const override;

};

OLIVE_NAMESPACE_EXIT

#endif // OPENGLWORKER_H
