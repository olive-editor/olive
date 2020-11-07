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
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLVertexArrayObject>
#include <QThread>

#include "render/backend/renderer.h"

OLIVE_NAMESPACE_ENTER

class OpenGLRenderer : public Renderer
{
  Q_OBJECT
public:
  OpenGLRenderer(QObject* parent = nullptr);

  virtual ~OpenGLRenderer() override;

  void Init(QOpenGLContext* existing_ctx);

  virtual bool Init() override;

public slots:
  virtual void PostInit() override;

  virtual void Destroy() override;

  virtual QVariant CreateNativeTexture(const VideoParams& p, void* data = nullptr, int linesize = 0) override;

  virtual void DestroyNativeTexture(QVariant texture) override;

  virtual void UploadToTexture(Texture* texture, void* data, int linesize) override;

  virtual void DownloadFromTexture(Texture* texture, void* data, int linesize) override;

  virtual TexturePtr ProcessShader(const OLIVE_NAMESPACE::Node* node,
                                   const OLIVE_NAMESPACE::TimeRange &range,
                                   const OLIVE_NAMESPACE::ShaderJob &job,
                                   const OLIVE_NAMESPACE::VideoParams &params) override;

  virtual TexturePtr TransformColor(Texture* texture, OLIVE_NAMESPACE::ColorProcessorPtr processor) override;

private:
  static GLint GetInternalFormat(PixelFormat::Format format);

  static GLenum GetPixelFormat(PixelFormat::Format format);

  static GLenum GetPixelType(PixelFormat::Format format);

  void PrepareInputTexture(bool bilinear);

  QOpenGLContext* context_;

  QOpenGLFunctions* functions_;

  QOffscreenSurface surface_;

  QOpenGLVertexArrayObject vao_;

  QOpenGLBuffer vert_vbo_;

  QOpenGLBuffer frag_vbo_;

  GLuint framebuffer_;

  QHash<QString, QOpenGLShaderProgram*> shader_cache_;

};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::OpenGLRenderer::TexturePtr);

#endif // OPENGLCONTEXT_H
