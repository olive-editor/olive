/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include <QVector2D>

#include "common/ocioutils.h"

namespace olive {

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

  return CreateTextureFromNativeHandle(v, params, type);
}

TexturePtr Renderer::CreateTexture(const VideoParams &params, const void *data, int linesize)
{
  return CreateTexture(params, Texture::k2D, data, linesize);
}

TexturePtr Renderer::InterlaceTexture(TexturePtr top, TexturePtr bottom, const VideoParams &params)
{
  color_cache_mutex_.lock();
  if (interlace_texture_.isNull()) {
    interlace_texture_ = CreateNativeShader(ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/interlace.frag"))));
  }
  color_cache_mutex_.unlock();

  ShaderJob job;
  job.InsertValue(QStringLiteral("top_tex_in"), NodeValue(NodeValue::kTexture, QVariant::fromValue(top)));
  job.InsertValue(QStringLiteral("bottom_tex_in"), NodeValue(NodeValue::kTexture, QVariant::fromValue(bottom)));
  job.InsertValue(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, QVector2D(params.effective_width(), params.effective_height())));

  TexturePtr output = CreateTexture(params);

  BlitToTexture(interlace_texture_, job, output.get());

  return output;
}

void Renderer::Destroy()
{
  color_cache_.clear();

  if (!interlace_texture_.isNull()) {
    DestroyNativeShader(interlace_texture_);
    interlace_texture_.clear();
  }

  DestroyInternal();
}

TexturePtr Renderer::CreateTextureFromNativeHandle(const QVariant &v, const VideoParams &params, Texture::Type type)
{
  if (v.isNull()) {
    return nullptr;
  }

  return std::make_shared<Texture>(this, v, params, type);
}

bool Renderer::GetColorContext(const ColorTransformJob &color_job, Renderer::ColorContext *ctx)
{
  QMutexLocker locker(&color_cache_mutex_);

  ColorContext& color_ctx = *ctx;

  QString proc_id = color_job.id();

  if (color_cache_.contains(proc_id)) {
    color_ctx = color_cache_.value(proc_id);
    return true;
  } else {
    // Create shader description
    QString ocio_func_name;
    if (color_job.GetFunctionName().isEmpty()) {
      ocio_func_name = "OCIODisplay";
    } else {
      ocio_func_name = color_job.GetFunctionName();
    }
    auto shader_desc = OCIO::GpuShaderDesc::CreateShaderDesc();
    shader_desc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_ES_3_0);
    shader_desc->setFunctionName(ocio_func_name.toUtf8());
    shader_desc->setResourcePrefix("ocio_");

    // Generate shader
    color_job.GetColorProcessor()->GetProcessor()->getDefaultGPUProcessor()->extractGpuShaderInfo(shader_desc);

    ShaderCode code;
    if (const Node *shader_src = color_job.CustomShaderSource()) {
      // Use shader code from associated node
      code = shader_src->GetShaderCode(color_job.CustomShaderID());
    } else {
      // Generate shader code using OCIO stub and our auto-generated name
      code = FileFunctions::ReadFileAsString(QStringLiteral(":shaders/colormanage.frag"));
    }

    code.set_frag_code(code.frag_code().arg(shader_desc->getShaderText()));

    // Try to compile shader
    color_ctx.compiled_shader = CreateNativeShader(code);

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

    color_cache_.insert(proc_id, color_ctx);

    return true;
  }
}

void Renderer::BlitColorManaged(const ColorTransformJob &color_job, Texture *destination, const VideoParams &params)
{
  ColorContext color_ctx;
  if (!GetColorContext(color_job, &color_ctx)) {
    return;
  }

  ShaderJob job;
  job.InsertValue(QStringLiteral("ove_maintex"), NodeValue(NodeValue::kTexture, QVariant::fromValue(color_job.GetInputTexture())));
  job.InsertValue(QStringLiteral("ove_mvpmat"), NodeValue(NodeValue::kMatrix, color_job.GetTransformMatrix()));
  job.InsertValue(QStringLiteral("ove_cropmatrix"), NodeValue(NodeValue::kMatrix, color_job.GetCropMatrix().inverted()));
  job.InsertValue(QStringLiteral("ove_maintex_alpha"), NodeValue(NodeValue::kInt, color_job.GetInputAlphaAssociation()));
  job.InsertValue(color_job.GetValues());
  job.SetAlphaChannelRequired(color_job.GetAlphaChannelRequired());

  foreach (const ColorContext::LUT& l, color_ctx.lut3d_textures) {
    job.InsertValue(l.name, NodeValue(NodeValue::kTexture, QVariant::fromValue(l.texture)));
    job.SetInterpolation(l.name, l.interpolation);
  }
  foreach (const ColorContext::LUT& l, color_ctx.lut1d_textures) {
    job.InsertValue(l.name, NodeValue(NodeValue::kTexture, QVariant::fromValue(l.texture)));
    job.SetInterpolation(l.name, l.interpolation);
  }

  if (destination) {
    BlitToTexture(color_ctx.compiled_shader, job, destination, color_job.IsClearDestinationEnabled());
  } else {
    Blit(color_ctx.compiled_shader, job, params, color_job.IsClearDestinationEnabled());
  }
}

}
