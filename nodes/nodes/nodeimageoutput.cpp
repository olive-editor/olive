#include "nodeimageoutput.h"

NodeImageOutput::NodeImageOutput(Clip *c) :
  OldEffectNode(c)
{
  NodeIO* input_texture = new NodeIO(this, "texture", tr("Texture"), true, false);
  input_texture->AddAcceptedNodeInput(olive::nodes::kTexture);
}

QString NodeImageOutput::name()
{
  return tr("Image Output");
}

QString NodeImageOutput::id()
{
  return "org.olivevideoeditor.Olive.imageoutput";
}

QString NodeImageOutput::category()
{
  return tr("Outputs");
}

QString NodeImageOutput::description()
{
  return tr("Used for outputting images outside of the node graph.");
}

EffectType NodeImageOutput::type()
{
  return EFFECT_TYPE_EFFECT;
}

olive::TrackType NodeImageOutput::subtype()
{
  return olive::kTypeVideo;
}

OldEffectNodePtr NodeImageOutput::Create(Clip *c)
{
  return std::make_shared<NodeImageOutput>(c);
}
