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

#ifndef OPENGLFRAMEBUFFER_H
#define OPENGLFRAMEBUFFER_H

#include <QOpenGLContext>

#include "opengltexture.h"

class OpenGLFramebuffer : public QObject
{
  Q_OBJECT
public:
  OpenGLFramebuffer();
  virtual ~OpenGLFramebuffer() override;

  Q_DISABLE_COPY_MOVE(OpenGLFramebuffer)

  void Create(QOpenGLContext *ctx);

  bool IsCreated() const;

  void Bind();

  void Release();

  void Attach(OpenGLTexturePtr texture);

  void AttachBackBuffer(OpenGLTexturePtr texture);

  void Detach();

  const GLuint& buffer() const;

public slots:
  void Destroy();

private:
  void AttachInternal(GLuint tex, bool clear);

  QOpenGLContext* context_;

  GLuint buffer_;

  OpenGLTexturePtr texture_;
};

#endif // OPENGLFRAMEBUFFER_H
