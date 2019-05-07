#include "nodemedia.h"

NodeMedia::NodeMedia(Clip* c) :
  OldEffectNode(c)
{
  NodeIO* matrix_input = new NodeIO(this, "matrix", tr("Matrix"), true, false);
  matrix_input->AddAcceptedNodeInput(olive::nodes::kMatrix);

  NodeIO* texture_output = new NodeIO(this, "texture", tr("Texture"), true, false);
  texture_output->SetOutputDataType(olive::nodes::kTexture);
}

QString NodeMedia::name()
{
  return tr("Media");
}

QString NodeMedia::id()
{
  return "org.olivevideoeditor.Olive.media";
}

QString NodeMedia::category()
{
  return tr("Inputs");
}

QString NodeMedia::description()
{
  return tr("Retrieve frames from a media source.");
}

EffectType NodeMedia::type()
{
  return EFFECT_TYPE_EFFECT;
}

olive::TrackType NodeMedia::subtype()
{
  return olive::kTypeVideo;
}

OldEffectNodePtr NodeMedia::Create(Clip *c)
{
  return std::make_shared<NodeMedia>(c);
}
