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

#ifndef IMAGE_H
#define IMAGE_H

#include <QOpenGLTexture>

#include "decoder/decoder.h"
#include "node/node.h"
#include "render/colorservice.h"

// FIXME: Test code only
#include "render/rendertexture.h"
#include "render/gl/shaderptr.h"
// End test code

/**
 * @brief A node that imports an image
 *
 * FIXME: This will likely be replaced by the Media node as the Media node will be set up to pull from various decoders
 *        from the beginning.
 */
class MediaInput : public Node
{
  Q_OBJECT
public:
  MediaInput();

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Category() override;
  virtual QString Description() override;

  virtual void Release() override;

  NodeOutput* texture_output();

  void SetFootage(Footage* f);

protected:
  virtual void Process(const rational &time) override;

private:
  NodeInput* footage_input_;

  NodeInput* matrix_input_;

  NodeOutput* texture_output_;

  RenderTexture texture_;

  DecoderPtr decoder_;

  ColorService color_service_;

};

#endif // IMAGE_H
