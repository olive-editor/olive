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

#ifndef RENDERCONTEXTTHREADWRAPPER_H
#define RENDERCONTEXTTHREADWRAPPER_H

#include <QThread>

#include "renderer.h"

OLIVE_NAMESPACE_ENTER

class RendererThreadWrapper : public Renderer
{
public:
  RendererThreadWrapper(Renderer* inner, QObject* parent = nullptr);

  virtual ~RendererThreadWrapper() override
  {
    Destroy();
    delete inner_;
  }

  virtual bool Init() override;

public slots:
  virtual void PostInit() override;

  virtual void DestroyInternal() override;

  virtual void ClearDestination(double r = 0.0, double g = 0.0, double b = 0.0, double a = 0.0) override;

  virtual QVariant CreateNativeTexture2D(int width, int height, OLIVE_NAMESPACE::VideoParams::Format format, int channel_count, const void* data = nullptr, int linesize = 0) override;
  virtual QVariant CreateNativeTexture3D(int width, int height, int depth, OLIVE_NAMESPACE::VideoParams::Format format, int channel_count, const void* data = nullptr, int linesize = 0) override;

  virtual void DestroyNativeTexture(QVariant texture) override;

  virtual QVariant CreateNativeShader(OLIVE_NAMESPACE::ShaderCode code) override;

  virtual void DestroyNativeShader(QVariant shader) override;

  virtual void UploadToTexture(OLIVE_NAMESPACE::Texture* texture, const void* data, int linesize) override;

  virtual void DownloadFromTexture(OLIVE_NAMESPACE::Texture* texture, void* data, int linesize) override;

protected slots:
  virtual void Blit(QVariant shader,
                    OLIVE_NAMESPACE::ShaderJob job,
                    OLIVE_NAMESPACE::Texture* destination,
                    OLIVE_NAMESPACE::VideoParams destination_params) override;

private:
  Renderer* inner_;

  QThread* thread_;

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERCONTEXTTHREADWRAPPER_H
