/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include <QDateTime>
#include <QThread>
#include <QTimer>
#include <QVector2D>

namespace olive {

Renderer::Renderer(QObject *parent) :
  QObject(parent)
{
}

TexturePtr Renderer::CreateTexture(const VideoParams &params, const void *data, int linesize)
{
  QVariant v;

  if (USE_TEXTURE_CACHE) {
    QMutexLocker locker(&texture_cache_lock_);
    for (auto it=texture_cache_.begin(); it!=texture_cache_.end(); it++) {
      if (it->width == params.effective_width()
          && it->height == params.effective_height()
          && it->depth == params.effective_depth()
          && it->format == params.format()
          && it->channel_count == params.channel_count()) {
        v = it->handle;
        texture_cache_.erase(it);
        break;
      }
    }
  }

  if (v.isNull()) {
    v = CreateNativeTexture(params.effective_width(), params.effective_height(), params.effective_depth(),
                            params.format(), params.channel_count(), data, linesize);
  } else if (data) {
    UploadToTexture(v, params, data, linesize);
  } else {
    this->Flush();
  }

  return CreateTextureFromNativeHandle(v, params);
}

void Renderer::DestroyTexture(Texture *texture)
{
  if (USE_TEXTURE_CACHE) {
    // HACK: Dirty, dirty hack. OpenGL uses "contexts" to store all of its data, and each context
    //       can only be used by the thread that created it. However there are also "shared contexts"
    //       where assets from one context can be used in another. We use shared contexts so that
    //       textures rendered in the background can be displayed on the screen, travelling from
    //       a background thread to the main UI thread. However, when that texture is destroyed, it
    //       comes back here to be placed in the texture cache. But that leads to a race condition
    //       because it will call the background thread's renderer in the main thread. Since all
    //       assets are shared, we could technically just get the texture to call "destroy" in the
    //       viewer's renderer instance, but that would mean all textures would end up stranded
    //       there unusable by the background renderer, negating the very advantage of the texture
    //       cache in the first place. Therefore, we simply allow the thread calling to happen, and
    //       use mutexes to prevent race conditions.
    //
    //       Presumably Vulkan would not have this issue because it allows for application-wide
    //       instances and multithreading.
    texture_cache_lock_.lock();
    texture_cache_.push_back({texture->params().effective_width(),
                              texture->params().effective_height(),
                              texture->params().effective_depth(),
                              texture->params().format(),
                              texture->params().channel_count(),
                              texture->id(),
                              QDateTime::currentMSecsSinceEpoch()});
    texture_cache_lock_.unlock();

    if (QThread::currentThread() == this->thread()) {
      ClearOldTextures();
    }
  } else {
    DestroyNativeTexture(texture->id());
  }
}

TexturePtr Renderer::InterlaceTexture(TexturePtr top, TexturePtr bottom, const VideoParams &params)
{
  color_cache_mutex_.lock();
  if (interlace_texture_.isNull()) {
    interlace_texture_ = CreateNativeShader(ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/interlace.frag"))));
  }
  color_cache_mutex_.unlock();

  ShaderJob job;
  job.Insert(QStringLiteral("top_tex_in"), NodeValue(NodeValue::kTexture, QVariant::fromValue(top)));
  job.Insert(QStringLiteral("bottom_tex_in"), NodeValue(NodeValue::kTexture, QVariant::fromValue(bottom)));
  job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, QVector2D(params.effective_width(), params.effective_height())));

  TexturePtr output = CreateTexture(params);

  BlitToTexture(interlace_texture_, job, output.get());

  return output;
}

QVariant Renderer::GetDefaultShader()
{
  QMutexLocker locker(&color_cache_mutex_);

  if (default_shader_.isNull()) {
    default_shader_ = CreateNativeShader(ShaderCode(QString(), QString()));
  }

  return default_shader_;
}

void Renderer::Destroy()
{
  if (!default_shader_.isNull()) {
    DestroyNativeShader(default_shader_);
    default_shader_.clear();
  }

  color_cache_.clear();

  if (!interlace_texture_.isNull()) {
    DestroyNativeShader(interlace_texture_);
    interlace_texture_.clear();
  }

  for (auto it=texture_cache_.begin(); it!=texture_cache_.end(); it++) {
    DestroyNativeTexture(it->handle);
  }
  texture_cache_.clear();

  DestroyInternal();
}

TexturePtr Renderer::CreateTextureFromNativeHandle(const QVariant &v, const VideoParams &params)
{
  if (v.isNull()) {
    return nullptr;
  }

  return std::make_shared<Texture>(this, v, params);
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
      code = shader_src->GetShaderCode({color_job.CustomShaderID(), shader_desc->getShaderText()});
    } else {
      // Generate shader code using OCIO stub and our auto-generated name
      code = FileFunctions::ReadFileAsString(QStringLiteral(":shaders/colormanage.frag"));
      code.set_frag_code(code.frag_code().arg(shader_desc->getShaderText()));
    }

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
      color_ctx.lut3d_textures[i].texture = CreateTexture(VideoParams(edge_len, edge_len, edge_len, PixelFormat::F32, VideoParams::kRGBChannelCount), values);
      color_ctx.lut3d_textures[i].name = sampler_name;
      color_ctx.lut3d_textures[i].interpolation = (interpolation == OCIO::INTERP_NEAREST) ? Texture::kNearest : Texture::kLinear;
    }

    color_ctx.lut1d_textures.resize(shader_desc->getNumTextures());
    for (unsigned int i=0; i<shader_desc->getNumTextures(); i++) {
      const char* tex_name = nullptr;
      const char* sampler_name = nullptr;
      unsigned int width = 0, height = 0;
      OCIO::GpuShaderDesc::TextureType channel = OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL;
#if OCIO_VERSION_HEX >= 0x02030000
      OCIO::GpuShaderDesc::TextureDimensions dimensions = OCIO::GpuShaderDesc::TEXTURE_2D;
#endif
      OCIO::Interpolation interpolation = OCIO::INTERP_LINEAR;

      shader_desc->getTexture(i, tex_name, sampler_name, width, height, channel,
#if OCIO_VERSION_HEX >= 0x02030000
                              // OCIO::GpuShaderDesc::TextureDimensions
                              dimensions,
#endif
                              interpolation);

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
      color_ctx.lut1d_textures[i].texture = CreateTexture(VideoParams(width, height, PixelFormat::F32, (channel == OCIO::GpuShaderDesc::TEXTURE_RED_CHANNEL) ? 1 : VideoParams::kRGBChannelCount), values);
      color_ctx.lut1d_textures[i].name = sampler_name;
      color_ctx.lut1d_textures[i].interpolation = (interpolation == OCIO::INTERP_NEAREST) ? Texture::kNearest : Texture::kLinear;
    }

    color_cache_.insert(proc_id, color_ctx);

    return true;
  }
}

void Renderer::ClearOldTextures()
{
  QMutexLocker locker(&texture_cache_lock_);

  for (auto it=texture_cache_.begin(); it!=texture_cache_.end(); ) {
    if (it->accessed < QDateTime::currentMSecsSinceEpoch() - MAX_TEXTURE_LIFE) {
      DestroyNativeTexture(it->handle);
      it = texture_cache_.erase(it);
    } else {
      it++;
    }
  }
}

void Renderer::BlitColorManaged(const ColorTransformJob &color_job, Texture *destination, const VideoParams &params)
{
  ColorContext color_ctx;
  if (!GetColorContext(color_job, &color_ctx)) {
    return;
  }

  ShaderJob job;
  job.Insert(QStringLiteral("ove_maintex"), color_job.GetInputTexture());
  job.Insert(QStringLiteral("ove_mvpmat"), NodeValue(NodeValue::kMatrix, color_job.GetTransformMatrix()));
  job.Insert(QStringLiteral("ove_cropmatrix"), NodeValue(NodeValue::kMatrix, color_job.GetCropMatrix().inverted()));
  job.Insert(QStringLiteral("ove_maintex_alpha"), NodeValue(NodeValue::kInt, int(color_job.GetInputAlphaAssociation())));
  job.Insert(QStringLiteral("ove_force_opaque"), NodeValue(NodeValue::kBoolean, color_job.GetForceOpaque()));
  job.Insert(color_job.GetValues());

  foreach (const ColorContext::LUT& l, color_ctx.lut3d_textures) {
    job.Insert(l.name, NodeValue(NodeValue::kTexture, QVariant::fromValue(l.texture)));
    job.SetInterpolation(l.name, l.interpolation);
  }
  foreach (const ColorContext::LUT& l, color_ctx.lut1d_textures) {
    job.Insert(l.name, NodeValue(NodeValue::kTexture, QVariant::fromValue(l.texture)));
    job.SetInterpolation(l.name, l.interpolation);
  }

  if (destination) {
    BlitToTexture(color_ctx.compiled_shader, job, destination, color_job.IsClearDestinationEnabled());
  } else {
    Blit(color_ctx.compiled_shader, job, params, color_job.IsClearDestinationEnabled());
  }
}

}
