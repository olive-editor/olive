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

#ifndef FRAMEBUFFEROBJECT_H
#define FRAMEBUFFEROBJECT_H

#include <QOpenGLContext>

#include "pixelformat.h"

/**
 * @brief A class for managing an OpenGL framebuffer and attached texture
 *
 * The GPU pipeline will frequently need framebuffers for drawing onto textures. The TextureBuffer is a convenience
 * class to handle the creation, management, and destruction of a framebuffer with a texture attachment.
 *
 * After creating a new TextureBuffer, call Create() when a valid OpenGL context is available.
 */
class TextureBuffer
{
public:
  TextureBuffer();
  ~TextureBuffer();

  bool IsCreated();
  void Create(QOpenGLContext* ctx, const olive::PixelFormat& format, int width, int height, void *data = nullptr);
  void Destroy();

  void Upload(void *data);

  const GLuint& buffer() const;
  const GLuint& texture() const;

  void BindBuffer() const;
  void ReleaseBuffer() const;

  void BindTexture() const;
  void ReleaseTexture() const;
private:
  QOpenGLContext* ctx_;
  GLuint buffer_;
  GLuint texture_;

  int width_;
  int height_;
  olive::PixelFormat format_;
};

#endif // FRAMEBUFFEROBJECT_H
