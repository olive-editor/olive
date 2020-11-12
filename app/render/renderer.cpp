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
#include "render/colormanager.h"

OLIVE_NAMESPACE_ENTER

Renderer::Renderer(QObject *parent) :
  QObject(parent)
{

}

TexturePtr Renderer::CreateTexture(const VideoParams &params, Texture::Type type, Texture::ChannelFormat channel_format, const void *data, int linesize)
{
  QVariant v;

  if (type == Texture::k3D) {
    v = CreateNativeTexture3D(params.effective_width(), params.effective_height(),
                              params.effective_depth(), params.format(), channel_format, data, linesize);
  } else {
    v = CreateNativeTexture2D(params.effective_width(), params.effective_height(), params.format(),
                              channel_format, data, linesize);
  }

  if (v.isNull()) {
    return nullptr;
  }

  return std::make_shared<Texture>(this, v, params, type);
}

TexturePtr Renderer::CreateTexture(const VideoParams &params, const void *data, int linesize)
{
  return CreateTexture(params, Texture::k2D, Texture::kRGBA, data, linesize);
}

void Renderer::BlitColorManaged(ColorProcessorPtr color_processor, TexturePtr source, Texture *destination, bool flipped)
{
  BlitColorManagedInternal(color_processor, source, destination, destination->params(), flipped);
}

void Renderer::BlitColorManaged(ColorProcessorPtr color_processor, TexturePtr source, VideoParams params, bool flipped)
{
  BlitColorManagedInternal(color_processor, source, nullptr, params, flipped);
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
                                      "void main() {\n"
                                      "  fragColor = %1(texture(ove_maintex, ove_texcoord));\n"
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
      color_ctx.lut3d_textures[i].texture = CreateTexture(VideoParams(edge_len, edge_len, edge_len, PixelFormat::PIX_FMT_RGBA32F),
                                                          Texture::k3D, Texture::kRGB, values);
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
      color_ctx.lut1d_textures[i].texture = CreateTexture(VideoParams(width, height, PixelFormat::PIX_FMT_RGBA32F),
                                                          Texture::k2D,
                                                          (channel == OCIO::GpuShaderDesc::TEXTURE_RED_CHANNEL) ? Texture::kRedOnly : Texture::kRGB,
                                                          values);
      color_ctx.lut1d_textures[i].name = sampler_name;
      color_ctx.lut1d_textures[i].interpolation = (interpolation == OCIO::INTERP_NEAREST) ? Texture::kNearest : Texture::kLinear;
    }

    color_cache_.insert(color_processor->id(), color_ctx);

    return true;
  }
}

void Renderer::BlitColorManagedInternal(ColorProcessorPtr color_processor, TexturePtr source, Texture *destination, VideoParams params, bool flipped)
{
  ColorContext color_ctx;
  if (!GetColorContext(color_processor, &color_ctx)) {
    return;
  }

  ShaderJob job;
  job.InsertValue(QStringLiteral("ove_maintex"), ShaderValue(QVariant::fromValue(source), NodeParam::kTexture));
  foreach (const ColorContext::LUT& l, color_ctx.lut3d_textures) {
    job.InsertValue(l.name, ShaderValue(QVariant::fromValue(l.texture), NodeParam::kTexture));
    job.SetInterpolation(l.name, l.interpolation);
  }
  foreach (const ColorContext::LUT& l, color_ctx.lut1d_textures) {
    job.InsertValue(l.name, ShaderValue(QVariant::fromValue(l.texture), NodeParam::kTexture));
    job.SetInterpolation(l.name, l.interpolation);
  }

  if (flipped) {
    QMatrix4x4 mat;
    mat.scale(1, -1, 1);
    job.SetMatrix(mat);
  }

  if (destination) {
    BlitToTexture(color_ctx.compiled_shader, job, destination);
  } else {
    Blit(color_ctx.compiled_shader, job, params);
  }
}

OLIVE_NAMESPACE_EXIT
