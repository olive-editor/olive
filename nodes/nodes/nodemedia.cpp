#include "nodemedia.h"

NodeMedia::NodeMedia(Clip* c) :
  Node(c)
{
  EffectRow* matrix_input = new EffectRow(this, "matrix", tr("Matrix"), true, false);
  matrix_input->AddAcceptedNodeInput(olive::nodes::kMatrix);

  EffectRow* texture_output = new EffectRow(this, "texture", tr("Texture"), true, false);
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

NodePtr NodeMedia::Create(Clip *c)
{
  return std::make_shared<NodeMedia>(c);
}
