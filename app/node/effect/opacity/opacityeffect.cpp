#include "opacityeffect.h"

#include "node/math/math/math.h"
#include "widget/slider/floatslider.h"

namespace olive {

#define super Node

const QString OpacityEffect::kTextureInput = QStringLiteral("tex_in");
const QString OpacityEffect::kValueInput = QStringLiteral("opacity_in");

OpacityEffect::OpacityEffect()
{
  MathNode *math = new MathNode();

  math->SetOperation(MathNode::kOpMultiply);

  SetNodePositionInContext(math, QPointF(0, 0));

  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kValueInput, NodeValue::kFloat, 1.0);
  SetInputProperty(kValueInput, QStringLiteral("view"), FloatSlider::kPercentage);
  SetInputProperty(kValueInput, QStringLiteral("min"), 0.0);
  SetInputProperty(kValueInput, QStringLiteral("max"), 1.0);

  SetFlags(kVideoEffect);
  SetEffectInput(kTextureInput);
}

void OpacityEffect::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kValueInput, tr("Opacity"));
}

ShaderCode OpacityEffect::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/opacity.frag"));
}

void OpacityEffect::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;

  job.InsertValue(value);

  // If there's no texture, no need to run an operation
  if (!job.GetValue(kTextureInput).data().isNull()) {
    if (!qFuzzyCompare(job.GetValue(kValueInput).data().toDouble(), 1.0)) {
      job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);
      table->Push(NodeValue::kTexture, QVariant::fromValue(job), this);
    } else {
      // 1.0 float is a no-op, so just push the texture
      table->Push(job.GetValue(kTextureInput));
    }
  }
}

}
