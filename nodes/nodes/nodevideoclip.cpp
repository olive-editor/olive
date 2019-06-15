#include "nodevideoclip.h"

NodeVideoClip::NodeVideoClip(NodeGraph *parent) :
  NodeBlock(parent)
{
  texture_input_ = new NodeIO(this, "texture", tr("Texture"), true, false);
  texture_input_->AddAcceptedNodeInput(olive::nodes::kTexture);

  texture_output_ = new NodeIO(this, "texture", tr("Texture"), true, false);
  texture_output_->SetOutputDataType(olive::nodes::kTexture);
}

NodeIO *NodeVideoClip::texture_output()
{
  return texture_output_;
}
