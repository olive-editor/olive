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

#ifndef OPENGLFUNCTIONS_H
#define OPENGLFUNCTIONS_H

#include <QMatrix4x4>
#include <QMutex>
#include <QOpenGLFunctions>

#include "openglshader.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

class OpenGLRenderFunctions {
public:
  /**
   * @brief Draw texture on screen
   *
   * @param pipeline
   *
   * Shader to use for the texture drawing
   *
   * @param flipped
   *
   * Draw the texture vertically flipped (defaults to FALSE)
   *
   * @param matrix
   *
   * Transformation matrix to use when drawing (defaults to no transform)
   */
  static void Blit(OpenGLShaderPtr pipeline, bool flipped = false, QMatrix4x4 matrix = QMatrix4x4());
  static void Blit(OpenGLShader* pipeline, bool flipped = false, QMatrix4x4 matrix = QMatrix4x4());
  static void Blit(OpenGLShader* pipeline, GLenum mode, const QVector<GLfloat>& vert,
                   const QVector<GLfloat>& tex, QMatrix4x4 matrix = QMatrix4x4());

  static void OCIOBlit(OpenGLShaderPtr pipeline, GLuint lut, bool flipped = false, QMatrix4x4 matrix = QMatrix4x4());
  static void OCIOBlit(OpenGLShader* pipeline, GLuint lut, bool flipped = false, QMatrix4x4 matrix = QMatrix4x4());

  static void PrepareToDraw(QOpenGLFunctions* f);

  static GLint GetInternalFormat(const PixelFormat::Format& format);

  static GLenum GetPixelFormat(const PixelFormat::Format& format);

  static GLenum GetPixelType(const PixelFormat::Format& format);

};

OLIVE_NAMESPACE_EXIT

#endif // OPENGLFUNCTIONS_H
