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

#ifndef OPENGLTEXTURE_H
#define OPENGLTEXTURE_H

#include <memory>
#include <QOpenGLFunctions>

#include "codec/frame.h"
#include "common/constructors.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A class wrapper around an OpenGL texture
 */
class OpenGLTexture : public QObject
{
  Q_OBJECT
public:
  OpenGLTexture();
  virtual ~OpenGLTexture() override;

  DISABLE_COPY_MOVE(OpenGLTexture)

  void Create(QOpenGLContext* ctx, int width, int height, const PixelFormat::Format &format, const void *data = nullptr);
  void Create(QOpenGLContext* ctx, FramePtr frame);

  bool IsCreated() const;

  void Bind();

  void Release();

  const int& width() const;

  const int& height() const;

  const PixelFormat::Format &format() const;

  const GLuint& texture() const;

  void Upload(const void *data);

public slots:
  void Destroy();

private:
  void CreateInternal(QOpenGLContext *create_ctx, GLuint *tex, const void *data = nullptr);

  QOpenGLContext* created_ctx_;

  GLuint texture_;

  int width_;

  int height_;

  PixelFormat::Format format_;

};

using OpenGLTexturePtr = std::shared_ptr<OpenGLTexture>;

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::OpenGLTexturePtr)

#endif // OPENGLTEXTURE_H
