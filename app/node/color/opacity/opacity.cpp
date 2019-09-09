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
  opacity_input_->set_value(100);
  opacity_input_->set_minimum(0);
  opacity_input_->set_maximum(100);
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

    if (input_tex == nullptr) {
      return 0;
    }

    // Attach texture's back buffer as frame buffer
    renderer->buffer()->AttachBackBuffer(input_tex);
    renderer->buffer()->Bind();

    // Bind texture's front buffer to draw with
    input_tex->Bind();

    // Set opacity to value
    ShaderPtr pipeline = renderer->default_pipeline();
    pipeline->bind();
    pipeline->setUniformValue("opacity", opacity_input_->get_value(time).toFloat()*0.01f);
    pipeline->release();

    renderer->context()->functions()->glBlendFunc(GL_ONE, GL_ZERO);

    // Blit
    olive::gl::Blit(pipeline);

    // Reset to full opacity
    pipeline->bind();
    pipeline->setUniformValue("opacity", 1.0f);
    pipeline->release();

    input_tex->Release();
    renderer->buffer()->Release();
    renderer->buffer()->Detach();

    input_tex->SwapFrontAndBack();

    return QVariant::fromValue(input_tex);
  }

  return 0;
}

void OpacityNode::Retranslate()
{
  opacity_input_->set_name(tr("Opacity"));
}

NodeInput *OpacityNode::texture_input()
{
  return texture_input_;
}

NodeOutput *OpacityNode::texture_output()
{
  return texture_output_;
}
