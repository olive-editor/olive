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

#include "opacity.h"

#include "node/processor/renderer/renderer.h"
#include "render/gl/functions.h"
#include "render/rendertexture.h"

OpacityNode::OpacityNode()
{
  opacity_input_ = new NodeInput("opacity_in");
  opacity_input_->add_data_input(NodeParam::kFloat);
  AddParameter(opacity_input_);

  texture_input_ = new NodeInput("tex_in");
  texture_input_->add_data_input(NodeParam::kTexture);
  AddParameter(texture_input_);

  texture_output_ = new NodeOutput("tex_out");
  texture_output_->set_data_type(NodeParam::kTexture);
  AddParameter(texture_output_);
}

QString OpacityNode::Name()
{
  return tr("Opacity");
}

QString OpacityNode::Category()
{
  return tr("Color");
}

QString OpacityNode::Description()
{
  return tr("Adjust an image's opacity.");
}

QString OpacityNode::id()
{
  return "org.olivevideoeditor.Olive.opacity";
}

QVariant OpacityNode::Value(NodeOutput *output, const rational &time)
{
  // Find the current Renderer instance
  RenderInstance* renderer = RendererProcessor::CurrentInstance();

  // If nothing is available, don't return a texture
  if (renderer == nullptr) {
    return 0;
  }

  if (output == texture_output_) {
    RenderTexturePtr input_tex = texture_input_->get_value(time).value<RenderTexturePtr>();

    // Attach texture's back buffer as frame buffer
    renderer->buffer()->AttachBackBuffer(input_tex);
    renderer->buffer()->Bind();

    // Bind texture's front buffer to draw with
    input_tex->Bind();

    // Set opacity to value
    ShaderPtr pipeline = renderer->default_pipeline();
    pipeline->setUniformValue("opacity", opacity_input_->get_value(time).toFloat()*0.01f);

    // Blit
    olive::gl::Blit(renderer->default_pipeline());

    // Reset to full opacity
    pipeline->setUniformValue("opacity", 1.0f);

    input_tex->Release();
    renderer->buffer()->Release();
    renderer->buffer()->Detach();
  }

  return 0;
}
