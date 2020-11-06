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

#ifndef OPENGLCONTEXT_H
#define OPENGLCONTEXT_H

#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QThread>

#include "render/backend/rendercontext.h"

OLIVE_NAMESPACE_ENTER

class OpenGLContext : public RenderContext
{
  Q_OBJECT
public:
  OpenGLContext(QObject* parent = nullptr);

  virtual ~OpenGLContext() override;

  virtual bool Init() override;

public slots:
  virtual void PostInit() override;

  virtual void Destroy() override;

  virtual QVariant CreateTexture(const VideoParams& param, void* data, int linesize) override;

  virtual void DestroyTexture(QVariant texture) override;

  virtual void UploadToTexture(QVariant texture, void* data, int linesize) override;

  virtual void DownloadFromTexture(QVariant texture, void* data, int linesize) override;

  virtual VideoParams GetParamsFromTexture(QVariant texture) override;

private:
  static GLint GetInternalFormat(PixelFormat::Format format);

  static GLenum GetPixelFormat(PixelFormat::Format format);

  static GLenum GetPixelType(PixelFormat::Format format);

  QOpenGLContext* context_;

  QOpenGLFunctions* functions_;

  QOffscreenSurface surface_;

  QMap<GLuint, VideoParams> texture_params_;

};

OLIVE_NAMESPACE_EXIT

#endif // OPENGLCONTEXT_H
