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

#ifndef RENDERCONTEXTTHREADWRAPPER_H
#define RENDERCONTEXTTHREADWRAPPER_H

#include <QThread>

#include "renderer.h"

OLIVE_NAMESPACE_ENTER

/*class RendererThreadWrapper : public Renderer
{
public:
  RendererThreadWrapper(Renderer* inner, QObject* parent = nullptr);

  virtual ~RendererThreadWrapper() override
  {
    Destroy();
  }

  virtual bool Init() override;

public slots:
  virtual void PostInit() override{}

  virtual void Destroy() override;

  virtual QVariant CreateTexture(const VideoParams& param, void* data, int linesize) override;

  virtual void DestroyTexture(QVariant texture) override;

  virtual void UploadToTexture(QVariant texture, void* data, int linesize) override;

  virtual void DownloadFromTexture(QVariant texture, void* data, int linesize) override;

  virtual QVariant ProcessShader(const OLIVE_NAMESPACE::Node* node,
                                 const OLIVE_NAMESPACE::TimeRange &range,
                                 const OLIVE_NAMESPACE::ShaderJob &job,
                                 const OLIVE_NAMESPACE::VideoParams &params) override;

  virtual QVariant TransformColor(QVariant texture,
                                  OLIVE_NAMESPACE::ColorProcessorPtr processor) override;

  //virtual VideoParams GetParamsFromTexture(QVariant texture) override;

private:
  Renderer* inner_;

  QThread* thread_;

};*/

OLIVE_NAMESPACE_EXIT

#endif // RENDERCONTEXTTHREADWRAPPER_H
