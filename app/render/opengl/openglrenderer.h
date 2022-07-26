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

#ifndef OPENGLCONTEXT_H
#define OPENGLCONTEXT_H

#include <QOffscreenSurface>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLVertexArrayObject>
#include <QThread>
#include <QTimer>

#include "render/renderer.h"

namespace olive {

class OpenGLRenderer : public Renderer
{
  Q_OBJECT
public:
  OpenGLRenderer(QObject* parent = nullptr);

  virtual ~OpenGLRenderer() override;

  void Init(QOpenGLContext* existing_ctx);

  virtual bool Init() override;

  virtual void PostDestroy() override;

  virtual void PostInit() override;

  virtual void ClearDestination(olive::Texture *texture = nullptr, double r = 0.0, double g = 0.0, double b = 0.0, double a = 0.0) override;

  virtual QVariant CreateNativeShader(olive::ShaderCode code) override;

  virtual void DestroyNativeShader(QVariant shader) override;

  virtual void UploadToTexture(const QVariant &handle, const VideoParams &params, const void* data, int linesize) override;

  virtual void DownloadFromTexture(const QVariant &handle, const VideoParams &params, void* data, int linesize) override;

  virtual void Flush() override;

  virtual Color GetPixelFromTexture(olive::Texture *texture, const QPointF &pt) override;

protected:
  virtual void Blit(QVariant shader,
                    olive::ShaderJob job,
                    olive::Texture* destination,
                    olive::VideoParams destination_params,
                    bool clear_destination) override;

  virtual QVariant CreateNativeTexture(int width, int height, int depth, olive::VideoParams::Format format, int channel_count, const void* data = nullptr, int linesize = 0) override;

  virtual void DestroyNativeTexture(QVariant texture) override;

  virtual void DestroyInternal() override;

private:
  static GLint GetInternalFormat(VideoParams::Format format, int channel_layout);

  static GLenum GetPixelType(VideoParams::Format format);

  static GLenum GetPixelFormat(int channel_count);

  void AttachTextureAsDestination(const QVariant &texture);

  void DetachTextureAsDestination();

  void PrepareInputTexture(GLenum target, Texture::Interpolation interp);

  void ClearDestinationInternal(double r = 0.0, double g = 0.0, double b = 0.0, double a = 0.0);

  GLuint CompileShader(GLenum type, const QString &code);

  QOpenGLContext* context_;

  QOpenGLFunctions* functions_;

  QOffscreenSurface surface_;

  GLuint framebuffer_;

  struct TextureCacheKey {
    int width;
    int height;
    int depth;
    VideoParams::Format format;
    int channel_count;

    bool operator==(const TextureCacheKey &rhs) const
    {
      return width == rhs.width && height == rhs.height && depth == rhs.depth
          && format == rhs.format && channel_count == rhs.channel_count;
    }
  };

  QMap<GLuint, TextureCacheKey> texture_params_;

  static const int kTextureCacheMaxSize;

};

}

#endif // OPENGLCONTEXT_H
