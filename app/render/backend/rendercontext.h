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
#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

class RenderContext : public QObject
{
  Q_OBJECT
public:
  RenderContext(QObject* parent = nullptr);

  virtual ~RenderContext() override;

  virtual bool Init() = 0;

public slots:
  virtual void PostInit() = 0;

  virtual void Destroy() = 0;

  virtual QVariant CreateTexture(const OLIVE_NAMESPACE::VideoParams& param, void* data, int linesize) = 0;

  virtual void DestroyTexture(QVariant texture) = 0;

  virtual void UploadToTexture(QVariant texture, void* data, int linesize) = 0;

  virtual void DownloadFromTexture(QVariant texture, void* data, int linesize) = 0;

  virtual QVariant CreateShader();

  virtual VideoParams GetParamsFromTexture(QVariant texture) = 0;

  QVariant CreateTexture(const OLIVE_NAMESPACE::VideoParams& param);

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERCONTEXT_H
