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

#ifndef RENDERCONTEXTTHREADWRAPPER_H
#define RENDERCONTEXTTHREADWRAPPER_H

#include <QThread>

#include "renderer.h"

namespace olive {

class RendererThreadWrapper : public Renderer
{
public:
  RendererThreadWrapper(Renderer* inner, QObject* parent = nullptr);

  virtual ~RendererThreadWrapper() override
  {
    Destroy();
    PostDestroy();
    delete inner_;
  }

  virtual bool Init() override;

  virtual void PostDestroy() override {}

public slots:
  virtual void PostInit() override;

  virtual void DestroyInternal() override;

  virtual void ClearDestination(olive::Texture *texture = nullptr, double r = 0.0, double g = 0.0, double b = 0.0, double a = 0.0) override;

  virtual QVariant CreateNativeTexture2D(int width, int height, olive::VideoParams::Format format, int channel_count, const void* data = nullptr, int linesize = 0) override;
  virtual QVariant CreateNativeTexture3D(int width, int height, int depth, olive::VideoParams::Format format, int channel_count, const void* data = nullptr, int linesize = 0) override;

  virtual void DestroyNativeTexture(QVariant texture) override;

  virtual QVariant CreateNativeShader(olive::ShaderCode code) override;

  virtual void DestroyNativeShader(QVariant shader) override;

  virtual void UploadToTexture(olive::Texture* texture, const void* data, int linesize) override;

  virtual void DownloadFromTexture(olive::Texture* texture, void* data, int linesize) override;

  virtual void Flush() override;

  virtual Color GetPixelFromTexture(olive::Texture *texture, const QPointF &pt) override;

protected slots:
  virtual void Blit(QVariant shader,
                    olive::ShaderJob job,
                    olive::Texture* destination,
                    olive::VideoParams destination_params,
                    bool clear_destination) override;

private:
  Renderer* inner_;

  QThread* thread_;

};

}

#endif // RENDERCONTEXTTHREADWRAPPER_H
