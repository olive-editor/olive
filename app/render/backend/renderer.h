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

#ifndef RENDERCONTEXT_H
#define RENDERCONTEXT_H

#include <QObject>
#include <QVariant>

#include "common/define.h"
#include "common/timerange.h"
#include "node/node.h"
#include "render/colorprocessor.h"
#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

class Renderer : public QObject
{
  Q_OBJECT
public:
  Renderer(QObject* parent = nullptr);

  virtual bool Init() = 0;

  class Texture
  {
  public:
    Texture(Renderer* renderer, const QVariant& native, const VideoParams& param) :
      renderer_(renderer),
      params_(param),
      id_(native),
      meaningful_alpha_(true)
    {
    }

    ~Texture()
    {
      renderer_->DestroyNativeTexture(id_);
    }

    QVariant id() const
    {
      return id_;
    }

    const VideoParams& params() const
    {
      return params_;
    }

    void Upload(void* data, int linesize)
    {
      renderer_->UploadToTexture(this, data, linesize);
    }

    int width() const
    {
      return params_.width();
    }

    int height() const
    {
      return params_.height();
    }

    PixelFormat::Format format() const
    {
      return params_.format();
    }

    int divider() const
    {
      return params_.divider();
    }

    const rational& pixel_aspect_ratio() const
    {
      return params_.pixel_aspect_ratio();
    }

    bool has_meaningful_alpha() const
    {
      return meaningful_alpha_;
    }

    void set_has_meaningful_alpha(bool e)
    {
      meaningful_alpha_ = e;
    }

  private:
    Renderer* renderer_;

    VideoParams params_;

    QVariant id_;

    bool meaningful_alpha_;

  };

  using TexturePtr = std::shared_ptr<Texture>;

  TexturePtr CreateTexture(const VideoParams& param, void* data = nullptr, int linesize = 0);

  void BlitToTexture(QVariant shader,
                     OLIVE_NAMESPACE::ShaderJob job,
                     OLIVE_NAMESPACE::Renderer::Texture* destination)
  {
    Blit(shader, job, destination, destination->params());
  }

  void Blit(QVariant shader,
            OLIVE_NAMESPACE::ShaderJob job,
            OLIVE_NAMESPACE::VideoParams params)
  {
    Blit(shader, job, nullptr, params);
  }

  void BlitColorManaged(ColorProcessorPtr color_processor, TexturePtr source, Texture* destination);
  void BlitColorManaged(ColorProcessorPtr color_processor, TexturePtr source, VideoParams params);

public slots:
  virtual void PostInit() = 0;

  virtual void Destroy() = 0;

  virtual void ClearDestination(double r = 0.0, double g = 0.0, double b = 0.0, double a = 0.0) = 0;

  virtual QVariant CreateNativeTexture(OLIVE_NAMESPACE::VideoParams param, void* data = nullptr, int linesize = 0) = 0;

  virtual void DestroyNativeTexture(QVariant texture) = 0;

  virtual QVariant CreateNativeShader(OLIVE_NAMESPACE::ShaderCode code) = 0;

  virtual void DestroyNativeShader(QVariant shader) = 0;

  virtual void UploadToTexture(OLIVE_NAMESPACE::Renderer::Texture* texture, void* data, int linesize) = 0;

  virtual void DownloadFromTexture(OLIVE_NAMESPACE::Renderer::Texture* texture, void* data, int linesize) = 0;

protected slots:
  virtual void Blit(QVariant shader,
                    OLIVE_NAMESPACE::ShaderJob job,
                    OLIVE_NAMESPACE::Renderer::Texture* destination,
                    OLIVE_NAMESPACE::VideoParams destination_params) = 0;

private:
  struct ColorContext {
    QVariant shader;
    TexturePtr lut;
  };

  QHash<QString, ColorContext> color_cache_;

};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::Renderer::TexturePtr);

#endif // RENDERCONTEXT_H
