#include "blend.h"

BlendNode::BlendNode()
{
  base_input_ = new NodeInput("base_in");
  base_input_->add_data_input(NodeParam::kTexture);
  AddParameter(base_input_);

  blend_input_ = new NodeInput("blend_in");
  blend_input_->add_data_input(NodeParam::kTexture);
  AddParameter(blend_input_);

  texture_output_ = new NodeOutput("tex_out");
  texture_output_->set_data_type(NodeParam::kTexture);
  AddParameter(texture_output_);
}

QString BlendNode::Category()
{
  return tr("Blend");
}

NodeInput *BlendNode::base_input()
{
  return base_input_;
}

NodeInput *BlendNode::blend_input()
{
  return blend_input_;
}

NodeOutput *BlendNode::texture_output()
{
  return texture_output_;
}
