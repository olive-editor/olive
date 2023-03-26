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

NodeValue OpacityEffect::Value(const ValueParams &p) const
{
  // If there's no texture, no need to run an operation
  NodeValue texture = GetInputValue(p, kTextureInput);
  NodeValue value = GetInputValue(p, kValueInput);

  if (TexturePtr tex = texture.toTexture()) {
    if (TexturePtr opacity_tex = value.toTexture()) {
      ShaderJob job = CreateJob<ShaderJob>(p);
      job.SetShaderID(QStringLiteral("rgbmult"));
      return NodeValue(NodeValue::kTexture, tex->toJob(job), this);
    } else if (!qFuzzyCompare(value.toDouble(), 1.0)) {
      return NodeValue(NodeValue::kTexture, tex->toJob(CreateJob<ShaderJob>(p)), this);
    }
  }

  return texture;
}

}
