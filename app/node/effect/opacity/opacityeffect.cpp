#include "opacityeffect.h"

#include "node/math/math/math.h"
#include "widget/slider/floatslider.h"

namespace olive {

#define super Node

const QString OpacityEffect::kTextureInput = QStringLiteral("tex_in");
const QString OpacityEffect::kValueInput = QStringLiteral("opacity_in");

OpacityEffect::OpacityEffect()
{
  AddInput(kTextureInput, TYPE_TEXTURE, kInputFlagNotKeyframable);

  AddInput(kValueInput, TYPE_DOUBLE, 1.0);
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

ShaderCode GetRGBShaderCode(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/opacity_rgb.frag"));
}

ShaderCode GetNumberShaderCode(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/opacity.frag"));
}

value_t OpacityEffect::Value(const ValueParams &p) const
{
  // If there's no texture, no need to run an operation
  value_t texture = GetInputValue(p, kTextureInput);
  value_t value = GetInputValue(p, kValueInput);

  if (TexturePtr tex = texture.toTexture()) {
    if (TexturePtr opacity_tex = value.toTexture()) {
      ShaderJob job = CreateShaderJob(p, GetRGBShaderCode);
      job.SetShaderID(QStringLiteral("rgbmult"));
      return tex->toJob(job);
    } else if (!qFuzzyCompare(value.toDouble(), 1.0)) {
      return tex->toJob(CreateShaderJob(p, GetNumberShaderCode));
    }
  }

  return texture;
}

}
