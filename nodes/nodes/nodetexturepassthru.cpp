#include "nodetexturepassthru.h"

NodeTexturePassthru::NodeTexturePassthru(NodeGraph *c) :
  Node(c)
{
  texture_input_ = new NodeIO(this, "tex_in", tr("Texture"), true, false);
  texture_input_->AddAcceptedNodeInput(olive::nodes::kTexture);

  texture_output_ = new NodeIO(this, "tex_out", tr("Texture"), true, false);
  texture_output_->SetOutputDataType(olive::nodes::kTexture);
}

QString NodeTexturePassthru::name()
{
  return tr("Image Output");
}

QString NodeTexturePassthru::id()
{
  return "org.olivevideoeditor.Olive.imageoutput";
}

void NodeTexturePassthru::Process(const rational &time)
{
  Q_UNUSED(time)

  texture_output_->SetValue(texture_input_->GetValue());
}

NodeIO *NodeTexturePassthru::texture_input()
{
  return texture_input_;
}

NodeIO *NodeTexturePassthru::texture_output()
{
  return texture_output_;
}
