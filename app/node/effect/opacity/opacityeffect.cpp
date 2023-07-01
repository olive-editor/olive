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

  SetFlag(kVideoEffect);
  SetEffectInput(kTextureInput);
}

void OpacityEffect::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kValueInput, tr("Opacity"));
}

ShaderCode OpacityEffect::GetShaderCode(const ShaderRequest &request) const
{
  if (request.id == QStringLiteral("rgbmult")) {
    return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/opacity_rgb.frag"));
  } else {
    return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/opacity.frag"));
  }
}

void OpacityEffect::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  // If there's no texture, no need to run an operation
  if (TexturePtr tex = value[kTextureInput].toTexture()) {
    if (TexturePtr opacity_tex = value[kValueInput].toTexture()) {
      ShaderJob job(value);
      job.SetShaderID(QStringLiteral("rgbmult"));
      table->Push(NodeValue::kTexture, tex->toJob(job), this);
    } else if (!qFuzzyCompare(value[kValueInput].toDouble(), 1.0)) {
      table->Push(NodeValue::kTexture, tex->toJob(ShaderJob(value)), this);
    } else {
      // 1.0 float is a no-op, so just push the texture
      table->Push(value[kTextureInput]);
    }
  }
}

}
