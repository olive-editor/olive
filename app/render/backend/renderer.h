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

  virtual ~Renderer() override;

  virtual bool Init() = 0;

  class Texture
  {
  public:
    Texture(Renderer* renderer, const QVariant& native, const VideoParams& param) :
      renderer_(renderer),
      params_(param),
      id_(native)
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

    void Upload(void* data, int linesize = 0)
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

  private:
    Renderer* renderer_;

    VideoParams params_;

    QVariant id_;

  };

  using TexturePtr = std::shared_ptr<Texture>;

  TexturePtr CreateTexture(const VideoParams& param, void* data = nullptr, int linesize = 0);

public slots:
  virtual void PostInit() = 0;

  virtual void Destroy() = 0;

  virtual QVariant CreateNativeTexture(const VideoParams& param, void* data = nullptr, int linesize = 0) = 0;

  virtual void DestroyNativeTexture(QVariant texture) = 0;

  virtual QVariant CreateNativeShader(const ShaderCode& code) = 0;

  virtual void DestroyNativeShader(QVariant shader) = 0;

  virtual void UploadToTexture(Texture* texture, void* data, int linesize) = 0;

  virtual void DownloadFromTexture(Texture* texture, void* data, int linesize) = 0;

  virtual TexturePtr ProcessShader(const OLIVE_NAMESPACE::Node* node,
                                   const OLIVE_NAMESPACE::TimeRange &range,
                                   const OLIVE_NAMESPACE::ShaderJob &job,
                                   const OLIVE_NAMESPACE::VideoParams &params) = 0;

  virtual TexturePtr TransformColor(Texture* texture, OLIVE_NAMESPACE::ColorProcessorPtr processor) = 0;

  virtual void Render() = 0;

  virtual void RenderToTexture(Texture* destination) = 0;

private:


};

OLIVE_NAMESPACE_EXIT

#endif // RENDERCONTEXT_H
