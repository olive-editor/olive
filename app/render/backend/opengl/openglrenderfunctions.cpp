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

#include "openglrenderfunctions.h"

#include <QOpenGLExtraFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

OLIVE_NAMESPACE_ENTER

const GLfloat blit_vertices[] = {
  -1.0f, -1.0f, 0.0f,
  1.0f, -1.0f, 0.0f,
  1.0f, 1.0f, 0.0f,

  -1.0f, -1.0f, 0.0f,
  -1.0f, 1.0f, 0.0f,
  1.0f, 1.0f, 0.0f
};

const GLfloat blit_texcoords[] = {
  0.0, 0.0,
  1.0, 0.0,
  1.0, 1.0,

  0.0, 0.0,
  0.0, 1.0,
  1.0, 1.0
};

const GLfloat flipped_blit_texcoords[] = {
  0.0, 1.0,
  1.0, 1.0,
  1.0, 0.0,

  0.0, 1.0,
  0.0, 0.0,
  1.0, 0.0
};

/**
 * @brief Set up texture parameters and mipmap for drawing
 *
 * Internal function used just before drawing to allow mipmapped bilinear filtering when drawing textures small.
 *
 * @param f
 *
 * Currently active QOpenGLFunctions object (use context()->functions() if unsure).
 */
void OpenGLRenderFunctions::PrepareToDraw(QOpenGLFunctions* f) {
  f->glGenerateMipmap(GL_TEXTURE_2D);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

GLint OpenGLRenderFunctions::GetInternalFormat(const PixelFormat::Format &format)
{
  switch (format) {
  case PixelFormat::PIX_FMT_RGB8:
    return GL_RGB8;
  case PixelFormat::PIX_FMT_RGBA8:
    return GL_RGBA8;
  case PixelFormat::PIX_FMT_RGB16U:
    return GL_RGB16;
  case PixelFormat::PIX_FMT_RGBA16U:
    return GL_RGBA16;
  case PixelFormat::PIX_FMT_RGB16F:
    return GL_RGB16F;
  case PixelFormat::PIX_FMT_RGBA16F:
    return GL_RGBA16F;
  case PixelFormat::PIX_FMT_RGB32F:
    return GL_RGB32F;
  case PixelFormat::PIX_FMT_RGBA32F:
    return GL_RGBA32F;

  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    break;
  }

  return GL_INVALID_VALUE;
}

GLenum OpenGLRenderFunctions::GetPixelFormat(const PixelFormat::Format &format)
{
  if (PixelFormat::FormatHasAlphaChannel(format)) {
    return GL_RGBA;
  } else {
    return GL_RGB;
  }
}

GLenum OpenGLRenderFunctions::GetPixelType(const PixelFormat::Format &format)
{
  switch (format) {
  case PixelFormat::PIX_FMT_RGB8:
  case PixelFormat::PIX_FMT_RGBA8:
    return GL_UNSIGNED_BYTE;
  case PixelFormat::PIX_FMT_RGB16U:
  case PixelFormat::PIX_FMT_RGBA16U:
    return GL_UNSIGNED_SHORT;
  case PixelFormat::PIX_FMT_RGB16F:
  case PixelFormat::PIX_FMT_RGBA16F:
    return GL_HALF_FLOAT;
  case PixelFormat::PIX_FMT_RGB32F:
  case PixelFormat::PIX_FMT_RGBA32F:
    return GL_FLOAT;

  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    break;
  }

  return GL_INVALID_VALUE;
}

void OpenGLRenderFunctions::Blit(OpenGLShaderPtr pipeline, bool flipped, QMatrix4x4 matrix)
{
  Blit(pipeline.get(), flipped, matrix);
}

void OpenGLRenderFunctions::Blit(OpenGLShader *pipeline, bool flipped, QMatrix4x4 matrix)
{
  // FIXME: is currentContext() reliable here?
  QOpenGLFunctions* func = QOpenGLContext::currentContext()->functions();

  PrepareToDraw(func);

  QOpenGLVertexArrayObject m_vao;
  m_vao.create();
  m_vao.bind();

  QOpenGLBuffer m_vbo;
  m_vbo.create();
  m_vbo.bind();
  m_vbo.allocate(blit_vertices, 18 * static_cast<int>(sizeof(GLfloat)));
  m_vbo.release();

  QOpenGLBuffer m_vbo2;
  m_vbo2.create();
  m_vbo2.bind();
  m_vbo2.allocate(flipped ? flipped_blit_texcoords : blit_texcoords, 12 * static_cast<int>(sizeof(GLfloat)));
  m_vbo2.release();

  pipeline->bind();

  pipeline->setUniformValue("ove_mvpmat", matrix);
  pipeline->setUniformValue("ove_maintex", 0);

  GLuint vertex_location = static_cast<GLuint>(pipeline->attributeLocation("a_position"));
  m_vbo.bind();
  func->glEnableVertexAttribArray(vertex_location);
  func->glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  m_vbo.release();

  GLuint tex_location = static_cast<GLuint>(pipeline->attributeLocation("a_texcoord"));
  m_vbo2.bind();
  func->glEnableVertexAttribArray(tex_location);
  func->glVertexAttribPointer(tex_location, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  m_vbo2.release();

  func->glDrawArrays(GL_TRIANGLES, 0, 6);

  pipeline->release();

  m_vbo2.destroy();
  m_vbo.destroy();
  m_vao.release();
  m_vao.destroy();
}

void OpenGLRenderFunctions::OCIOBlit(OpenGLShaderPtr pipeline, GLuint lut, bool flipped, QMatrix4x4 matrix)
{
  OCIOBlit(pipeline.get(), lut, flipped, matrix);
}

void OpenGLRenderFunctions::OCIOBlit(OpenGLShader *pipeline,
                                     GLuint lut,
                                     bool flipped,
                                     QMatrix4x4 matrix)
{
  QOpenGLContext* ctx = QOpenGLContext::currentContext();
  QOpenGLExtraFunctions* xf = ctx->extraFunctions();

  xf->glActiveTexture(GL_TEXTURE1);
  xf->glBindTexture(GL_TEXTURE_3D, lut);
  xf->glActiveTexture(GL_TEXTURE0);

  pipeline->bind();

  pipeline->setUniformValue("ove_ociolut", 1);

  Blit(pipeline, flipped, matrix);

  pipeline->release();

  xf->glActiveTexture(GL_TEXTURE1);
  xf->glBindTexture(GL_TEXTURE_3D, 0);
  xf->glActiveTexture(GL_TEXTURE0);
}

OLIVE_NAMESPACE_EXIT
