#include "opacityeffect.h"

#include "node/math/math/math.h"
#include "widget/slider/floatslider.h"

namespace olive {

#define super NodeGroup

OpacityEffect::OpacityEffect()
{
  MathNode *math = new MathNode();

  math->SetOperation(MathNode::kOpMultiply);

  SetNodePositionInContext(math, QPointF(0, 0));

  tex_in_pass_ = AddInputPassthrough(NodeInput(math, MathNode::kParamAIn), InputFlags(kInputFlagNotKeyframable));
  SetInputDataType(tex_in_pass_, NodeValue::kTexture);

  value_in_pass_ = AddInputPassthrough(NodeInput(math, MathNode::kParamBIn));
  SetInputProperty(value_in_pass_, QStringLiteral("view"), FloatSlider::kPercentage);
  SetInputProperty(value_in_pass_, QStringLiteral("min"), 0.0);
  SetInputProperty(value_in_pass_, QStringLiteral("max"), 1.0);
  math->SetStandardValue(MathNode::kParamBIn, 1.0);

  SetOutputPassthrough(math);
}

void OpacityEffect::Retranslate()
{
  super::Retranslate();

  SetInputName(tex_in_pass_, tr("Texture"));
  SetInputName(value_in_pass_, tr("Opacity"));
}

}
