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

#ifndef OPENGLBACKEND_H
#define OPENGLBACKEND_H

#include "render/backend/renderbackend.h"

OLIVE_NAMESPACE_ENTER

class OpenGLBackend : public RenderBackend
{
public:
  OpenGLBackend(QObject* parent = nullptr);

protected:
  virtual void TextureToFrame(const QVariant& texture, FramePtr frame) const override;

  virtual NodeValue FrameToTexture(FramePtr frame) const override;

  virtual void ProcessNodeEvent(const Node *node, const TimeRange &range, NodeValueDatabase &input_params, NodeValueTable &output_params) const override;

};

OLIVE_NAMESPACE_EXIT

#endif // OPENGLBACKEND_H
