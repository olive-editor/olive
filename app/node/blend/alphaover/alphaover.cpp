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

#include "alphaover.h"

#include "node/processor/videorenderer/videorenderer.h"
#include "render/gl/functions.h"
#include "render/rendertexture.h"
#include "render/gl/shadergenerators.h"

AlphaOverBlend::AlphaOverBlend()
{

}

QString AlphaOverBlend::Name()
{
  return tr("Alpha Over");
}

QString AlphaOverBlend::id()
{
  return "org.olivevideoeditor.Olive.alphaoverblend";
}

QString AlphaOverBlend::Description()
{
  return tr("A blending node that composites one texture over another using its alpha channel.");
}

void AlphaOverBlend::Release()
{
}

QVariant AlphaOverBlend::Value(NodeOutput *param, const rational &time)
{
  // Find the current Renderer instance
  RenderInstance* renderer = VideoRendererProcessor::CurrentInstance();

  // If nothing is available, don't return a texture
  if (renderer == nullptr) {
    return 0;
  }

  // The only parameter should be texture output, but for future proofing we put this here
  if (param == texture_output()) {
    RenderTexturePtr base = base_input()->get_value(time).value<RenderTexturePtr>();
    RenderTexturePtr blend = blend_input()->get_value(time).value<RenderTexturePtr>();

    if (base == nullptr && blend == nullptr) {
      return 0;
    } else if (base == nullptr) {
      return QVariant::fromValue(blend);
    } else if (blend == nullptr) {
      return QVariant::fromValue(base);
    }

    // Attach framebuffer to the backbuffer of base
    renderer->buffer()->Attach(base);
    renderer->buffer()->Bind();

    // Bind blend
    blend->Bind();

    // Set compositing strategy to alpha over
    renderer->context()->functions()->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Draw blend on base
    olive::gl::Blit(renderer->default_pipeline());

    // Release all
    blend->Release();
    renderer->buffer()->Release();
    renderer->buffer()->Detach();

    // Return base texture which now has blend composited on top
    // NOTE: Blend texture will be implicitly deleted here (if it's not used anywhere else)
    return QVariant::fromValue(base);
  }

  return 0;
}
