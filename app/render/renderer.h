/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include <QObject>
#include <QVariant>

#include "common/define.h"
#include "common/timerange.h"
#include "node/node.h"
#include "render/colorprocessor.h"
#include "render/videoparams.h"
#include "texture.h"

OLIVE_NAMESPACE_ENTER

class ShaderJob;

class Renderer : public QObject
{
  Q_OBJECT
public:
  Renderer(QObject* parent = nullptr);

  virtual bool Init() = 0;

  TexturePtr CreateTexture(const VideoParams& params, Texture::Type type, const void* data = nullptr, int linesize = 0);
  TexturePtr CreateTexture(const VideoParams& params, const void *data = nullptr, int linesize = 0);

  void BlitToTexture(QVariant shader,
                     OLIVE_NAMESPACE::ShaderJob job,
                     OLIVE_NAMESPACE::Texture* destination)
  {
    Blit(shader, job, destination, destination->params());
  }

  void Blit(QVariant shader,
            OLIVE_NAMESPACE::ShaderJob job,
            OLIVE_NAMESPACE::VideoParams params)
  {
    Blit(shader, job, nullptr, params);
  }

  void BlitColorManaged(ColorProcessorPtr color_processor, TexturePtr source, bool source_is_premultiplied, Texture* destination, const QMatrix4x4& matrix = QMatrix4x4());
  void BlitColorManaged(ColorProcessorPtr color_processor, TexturePtr source, bool source_is_premultiplied, VideoParams params, const QMatrix4x4& matrix = QMatrix4x4());

  void Destroy();

public slots:
  virtual void PostInit() = 0;

  virtual void DestroyInternal() = 0;

  virtual void ClearDestination(double r = 0.0, double g = 0.0, double b = 0.0, double a = 0.0) = 0;

  virtual QVariant CreateNativeTexture2D(int width, int height, OLIVE_NAMESPACE::VideoParams::Format format, int channel_count, const void* data = nullptr, int linesize = 0) = 0;
  virtual QVariant CreateNativeTexture3D(int width, int height, int depth, OLIVE_NAMESPACE::VideoParams::Format format, int channel_count, const void* data = nullptr, int linesize = 0) = 0;

  virtual void DestroyNativeTexture(QVariant texture) = 0;

  virtual QVariant CreateNativeShader(OLIVE_NAMESPACE::ShaderCode code) = 0;

  virtual void DestroyNativeShader(QVariant shader) = 0;

  virtual void UploadToTexture(OLIVE_NAMESPACE::Texture* texture, const void* data, int linesize) = 0;

  virtual void DownloadFromTexture(OLIVE_NAMESPACE::Texture* texture, void* data, int linesize) = 0;

protected slots:
  virtual void Blit(QVariant shader,
                    OLIVE_NAMESPACE::ShaderJob job,
                    OLIVE_NAMESPACE::Texture* destination,
                    OLIVE_NAMESPACE::VideoParams destination_params) = 0;

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

  enum AlphaAssociated {
    kAlphaNone,
    kAlphaUnassociated,
    kAlphaAssociated
  };

  bool GetColorContext(ColorProcessorPtr color_processor, ColorContext* ctx);

  void BlitColorManagedInternal(ColorProcessorPtr color_processor, TexturePtr source,
                                bool source_is_premultiplied,
                                Texture* destination, VideoParams params, const QMatrix4x4 &matrix);

  QHash<QString, ColorContext> color_cache_;

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERCONTEXT_H
