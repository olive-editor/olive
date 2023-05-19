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

#ifndef RENDERCONTEXT_H
#define RENDERCONTEXT_H

#include <QMutex>
#include <QObject>
#include <QVariant>

#include "common/define.h"
#include "node/node.h"
#include "render/colorprocessor.h"
#include "render/job/colortransformjob.h"
#include "render/videoparams.h"
#include "texture.h"

namespace olive {

class ShaderJob;

class Renderer : public QObject
{
  Q_OBJECT
public:
  Renderer(QObject* parent = nullptr);

  virtual bool Init() = 0;

  TexturePtr CreateTexture(const VideoParams& params, const void *data = nullptr, int linesize = 0);

  void DestroyTexture(Texture *texture);

  void BlitToTexture(QVariant shader,
                     olive::ShaderJob job,
                     olive::Texture* destination,
                     bool clear_destination = true)
  {
    Blit(shader, job, destination, destination->params(), clear_destination);
  }

  void Blit(QVariant shader,
            olive::ShaderJob job,
            olive::VideoParams params,
            bool clear_destination = true)
  {
    Blit(shader, job, nullptr, params, clear_destination);
  }

  void BlitColorManaged(const ColorTransformJob &color_job, Texture* destination, const VideoParams &params);
  void BlitColorManaged(const ColorTransformJob &job, Texture* destination)
  {
    BlitColorManaged(job, destination, destination->params());
  }
  void BlitColorManaged(const ColorTransformJob &job, const VideoParams &params)
  {
    BlitColorManaged(job, nullptr, params);
  }

  TexturePtr InterlaceTexture(TexturePtr top, TexturePtr bottom, const VideoParams &params);

  QVariant GetDefaultShader();

  void Destroy();

  virtual void PostDestroy() = 0;

  virtual void PostInit() = 0;

  virtual void ClearDestination(olive::Texture *texture = nullptr, double r = 0.0, double g = 0.0, double b = 0.0, double a = 1.0) = 0;

  virtual QVariant CreateNativeShader(olive::ShaderCode code) = 0;

  virtual void DestroyNativeShader(QVariant shader) = 0;

  virtual void UploadToTexture(const QVariant &handle, const VideoParams &params, const void* data, int linesize) = 0;

  virtual void DownloadFromTexture(const QVariant &handle, const VideoParams &params, void* data, int linesize) = 0;

  virtual void Flush() = 0;

  virtual Color GetPixelFromTexture(olive::Texture *texture, const QPointF &pt) = 0;

protected:
  virtual void Blit(QVariant shader,
                    olive::ShaderJob job,
                    olive::Texture* destination,
                    olive::VideoParams destination_params,
                    bool clear_destination) = 0;

  virtual QVariant CreateNativeTexture(int width, int height, int depth, PixelFormat format, int channel_count, const void* data = nullptr, int linesize = 0) = 0;

  virtual void DestroyNativeTexture(QVariant texture) = 0;

  virtual void DestroyInternal() = 0;

private:
  struct ColorContext {
    struct LUT {
      TexturePtr texture;
      Texture::Interpolation interpolation;
      QString name;
    };

    QVariant compiled_shader;
    QVector<LUT> lut3d_textures;
    QVector<LUT> lut1d_textures;

  };

  TexturePtr CreateTextureFromNativeHandle(const QVariant &v, const VideoParams &params);

  bool GetColorContext(const ColorTransformJob &color_job, ColorContext* ctx);

  void ClearOldTextures();

  QHash<QString, ColorContext> color_cache_;

  struct CachedTexture
  {
    int width;
    int height;
    int depth;
    PixelFormat format;
    int channel_count;
    QVariant handle;
    qint64 accessed;
  };

  static const int MAX_TEXTURE_LIFE = 5000;
  static const bool USE_TEXTURE_CACHE = true;
  std::list<CachedTexture> texture_cache_;

  QMutex color_cache_mutex_;

  QVariant default_shader_;

  QVariant interlace_texture_;

  QMutex texture_cache_lock_;

};

}

#endif // RENDERCONTEXT_H
