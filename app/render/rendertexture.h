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

#ifndef RENDERTEXTURE_H
#define RENDERTEXTURE_H

#include <memory>
#include <QOpenGLFunctions>

#include "pixelformat.h"

class RenderTexture : public QObject
{
public:
  RenderTexture();
  ~RenderTexture();
  RenderTexture(const RenderTexture& other) = delete;
  RenderTexture(RenderTexture&& other) = delete;
  RenderTexture& operator=(const RenderTexture& other) = delete;
  RenderTexture& operator=(RenderTexture&& other) = delete;

  void Create(QOpenGLContext* ctx, int width, int height, const olive::PixelFormat &format, void *data = nullptr);

  bool IsCreated() const;

  void Destroy();

  void Bind();

  void Release();

  const int& width() const;

  const int& height() const;

  const olive::PixelFormat &format() const;

  QOpenGLContext* context() const;

  const GLuint& texture() const;

  void Upload(void* data);

  void* Download() const;

private:
  QOpenGLContext* context_;

  GLuint texture_;

  int width_;

  int height_;

  olive::PixelFormat format_;
};

using RenderTexturePtr = std::shared_ptr<RenderTexture>;
Q_DECLARE_METATYPE(RenderTexturePtr)

#endif // RENDERTEXTURE_H
