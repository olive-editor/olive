/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "renderer.h"

#include <QFloat16>

#include "common/ocioutils.h"

OLIVE_NAMESPACE_ENTER

Renderer::Renderer(QObject *parent) :
  QObject(parent)
{

}

TexturePtr Renderer::CreateTexture(const VideoParams &params, Texture::Type type, const void *data, int linesize)
{
  QVariant v;

  if (type == Texture::k3D) {
    v = CreateNativeTexture3D(params.effective_width(), params.effective_height(),
                              params.effective_depth(), params.format(), params.channel_count(), data, linesize);
  } else {
    v = CreateNativeTexture2D(params.effective_width(), params.effective_height(), params.format(),
                              params.channel_count(), data, linesize);
  }

  if (v.isNull()) {
    return nullptr;
  }

  return std::make_shared<Texture>(this, v, params, type);
}

TexturePtr Renderer::CreateTexture(const VideoParams &params, const void *data, int linesize)
{
  return CreateTexture(params, Texture::k2D, data, linesize);
}

void Renderer::BlitColorManaged(ColorProcessorPtr color_processor, TexturePtr source, bool source_is_premultiplied, Texture *destination, const QMatrix4x4 &matrix)
{
  BlitColorManagedInternal(color_processor, source, source_is_premultiplied, destination, destination->params(), matrix);
}

void Renderer::BlitColorManaged(ColorProcessorPtr color_processor, TexturePtr source, bool source_is_premultiplied, VideoParams params, const QMatrix4x4& matrix)
{
  BlitColorManagedInternal(color_processor, source, source_is_premultiplied, nullptr, params, matrix);
}

void Renderer::Destroy()
{
  color_cache_.clear();

  DestroyInternal();
}

bool Renderer::GetColorContext(ColorProcessorPtr color_processor, Renderer::ColorContext *ctx)
{
  ColorContext& color_ctx = *ctx;

  if (color_cache_.contains(color_processor->id())) {
    color_ctx = color_cache_.value(color_processor->id());
    return true;
  } else {
    // Create shader description
    const char* ocio_func_name = "OCIODisplay";
    auto shader_desc = OCIO::GpuShaderDesc::CreateShaderDesc();
    shader_desc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);
    shader_desc->setFunctionName(ocio_func_name);
    shader_desc->setResourcePrefix("ocio_");

    // Generate shader
    color_processor->GetProcessor()->getDefaultGPUProcessor()->extractGpuShaderInfo(shader_desc);

    QString shader_frag;
    shader_frag.append(QStringLiteral("#version 150\n"
                                      "\n"
                                      "#ifdef GL_ES\n"
                                      "precision highp int;\n"
                                      "precision highp float;\n"
                                      "#endif\n"
                                      "\n"
                                      "// Main texture input\n"
                                      "uniform sampler2D ove_maintex;\n"
                                      "uniform int ove_maintex_alpha;\n"
                                      "\n"
                                      "// Macros defining `ove_maintex_alpha` state\n"
                                      "// Matches `AlphaAssociated` C++ enum\n"
                                      "#define ALPHA_NONE     0\n"
                                      "#define ALPHA_UNASSOC  1\n"
                                      "#define ALPHA_ASSOC    2\n"
                                      "\n"
                                      "// Macros so OCIO's shaders work on this GLSL version\n"
                                      "#define texture2D texture\n"
                                      "#define texture3D texture\n"
                                      "\n"
                                      "// Main texture coordinate\n"
                                      "in vec2 ove_texcoord;\n"
                                      "\n"
                                      "// Texture output\n"
                                      "out vec4 fragColor;\n"));
    shader_frag.append(shader_desc->getShaderText());
    shader_frag.append(QStringLiteral("\n"
                                      "// Alpha association functions\n"
                                      "vec4 assoc(vec4 c) {\n"
                                      "  return vec4(c.rgb * c.a, c.a);\n"
                                      "}\n"
                                      "\n"
                                      "vec4 reassoc(vec4 c) {\n"
                                      "  return (c.a == 0.0) ? c : assoc(c);\n"
                                      "}\n"
                                      "\n"
                                      "vec4 deassoc(vec4 c) {\n"
                                      "  return (c.a == 0.0) ? c : vec4(c.rgb / c.a, c.a);\n"
                                      "}\n"
                                      "\n"
                                      "void main() {\n"
                                      "  vec4 col = texture(ove_maintex, ove_texcoord);\n"
                                      "\n"
                                      "  // If alpha is associated, de-associate now\n"
                                      "  if (ove_maintex_alpha == ALPHA_ASSOC) {\n"
                                      "    col = deassoc(col);\n"
                                      "  }\n"
                                      "\n"
                                      "  // Perform color conversion\n"
                                      "  col = %1(col);\n"
                                      "\n"
                                      "  // Associate or re-associate here\n"
                                      "  if (ove_maintex_alpha == ALPHA_ASSOC) {\n"
                                      "    col = reassoc(col);\n"
                                      "  } else if (ove_maintex_alpha == ALPHA_UNASSOC) {\n"
                                      "    col = assoc(col);\n"
                                      "  }\n"
                                      "\n"
                                      "  fragColor = col;\n"
                                      "}\n").arg(ocio_func_name));

    // Try to compile shader
    color_ctx.compiled_shader = CreateNativeShader(ShaderCode(shader_frag,
                                                              FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/default.vert"))));

    if (color_ctx.compiled_shader.isNull()) {
      return false;
    }

    color_ctx.lut3d_textures.resize(shader_desc->getNum3DTextures());
    for (unsigned int i=0; i<shader_desc->getNum3DTextures(); i++) {
      const char* tex_name = nullptr;
      const char* sampler_name = nullptr;
      unsigned int edge_len = 0;
      OCIO::Interpolation interpolation = OCIO::INTERP_LINEAR;

      shader_desc->get3DTexture(i, tex_name, sampler_name, edge_len, interpolation);

      if (!tex_name || !*tex_name
          || !sampler_name || !*sampler_name
          || !edge_len) {
        qCritical() << "3D LUT texture data is corrupted";
        return false;
      }

      const float* values = nullptr;
      shader_desc->get3DTextureValues(i, values);
      if (!values) {
        qCritical() << "3D LUT texture values are missing";
        return false;
      }

      // Allocate 3D LUT
      color_ctx.lut3d_textures[i].texture = CreateTexture(VideoParams(edge_len, edge_len, edge_len, VideoParams::kFormatFloat32, VideoParams::kRGBChannelCount),
                                                          Texture::k3D, values);
      color_ctx.lut3d_textures[i].name = sampler_name;
      color_ctx.lut3d_textures[i].interpolation = (interpolation == OCIO::INTERP_NEAREST) ? Texture::kNearest : Texture::kLinear;
    }

    color_ctx.lut1d_textures.resize(shader_desc->getNumTextures());
    for (unsigned int i=0; i<shader_desc->getNumTextures(); i++) {
      const char* tex_name = nullptr;
      const char* sampler_name = nullptr;
      unsigned int width = 0, height = 0;
      OCIO::GpuShaderDesc::TextureType channel = OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL;
      OCIO::Interpolation interpolation = OCIO::INTERP_LINEAR;

      shader_desc->getTexture(i, tex_name, sampler_name, width, height, channel, interpolation);

      if (!tex_name || !*tex_name
          || !sampler_name || !*sampler_name
          || !width) {
        qCritical() << "1D LUT texture data is corrupted";
        return false;
      }

      const float* values = nullptr;
      shader_desc->getTextureValues(i, values);
      if (!values) {
        qCritical() << "1D LUT texture values are missing";
        return false;
      }

      // Allocate 1D LUT
      color_ctx.lut1d_textures[i].texture = CreateTexture(VideoParams(width, height, VideoParams::kFormatFloat32, (channel == OCIO::GpuShaderDesc::TEXTURE_RED_CHANNEL) ? 1 : VideoParams::kRGBChannelCount),
                                                          Texture::k2D,
                                                          values);
      color_ctx.lut1d_textures[i].name = sampler_name;
      color_ctx.lut1d_textures[i].interpolation = (interpolation == OCIO::INTERP_NEAREST) ? Texture::kNearest : Texture::kLinear;
    }

    color_cache_.insert(color_processor->id(), color_ctx);

    return true;
  }
}

void Renderer::BlitColorManagedInternal(ColorProcessorPtr color_processor, TexturePtr source,
                                        bool source_is_premultiplied, Texture *destination,
                                        VideoParams params, const QMatrix4x4& matrix)
{
  ColorContext color_ctx;
  if (!GetColorContext(color_processor, &color_ctx)) {
    return;
  }

  ShaderJob job;

  job.InsertValue(QStringLiteral("ove_maintex"), ShaderValue(QVariant::fromValue(source), NodeParam::kTexture));
  job.InsertValue(QStringLiteral("ove_mvpmat"), ShaderValue(matrix, NodeParam::kMatrix));

  AlphaAssociated associated;
  if (source->channel_count() == VideoParams::kRGBAChannelCount) {
    if (source_is_premultiplied) {
      // De-assoc/re-assoc required for color management
      associated = kAlphaAssociated;
    } else {
      // Just assoc at the end
      associated = kAlphaUnassociated;
    }
  } else {
    // No assoc/deassoc required
    associated = kAlphaNone;
  }
  job.InsertValue(QStringLiteral("ove_maintex_alpha"), ShaderValue(associated, NodeParam::kInt));

  foreach (const ColorContext::LUT& l, color_ctx.lut3d_textures) {
    job.InsertValue(l.name, ShaderValue(QVariant::fromValue(l.texture), NodeParam::kTexture));
    job.SetInterpolation(l.name, l.interpolation);
  }
  foreach (const ColorContext::LUT& l, color_ctx.lut1d_textures) {
    job.InsertValue(l.name, ShaderValue(QVariant::fromValue(l.texture), NodeParam::kTexture));
    job.SetInterpolation(l.name, l.interpolation);
  }

  if (destination) {
    BlitToTexture(color_ctx.compiled_shader, job, destination);
  } else {
    Blit(color_ctx.compiled_shader, job, params);
  }
}

OLIVE_NAMESPACE_EXIT
