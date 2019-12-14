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

#include "functions.h"

#include <QOpenGLExtraFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

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
void olive::gl::PrepareToDraw(QOpenGLFunctions* f) {
  f->glGenerateMipmap(GL_TEXTURE_2D);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void olive::gl::Blit(OpenGLShaderPtr pipeline, bool flipped, QMatrix4x4 matrix) {
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

void olive::gl::OCIOBlit(OpenGLShaderPtr pipeline,
                         GLuint lut,
                         bool flipped,
                         QMatrix4x4 matrix)
{
  QOpenGLContext* ctx = QOpenGLContext::currentContext();
  QOpenGLExtraFunctions* xf = ctx->extraFunctions();

  xf->glActiveTexture(GL_TEXTURE2);
  xf->glBindTexture(GL_TEXTURE_3D, lut);
  xf->glActiveTexture(GL_TEXTURE0);

  pipeline->bind();

  pipeline->setUniformValue("ove_ociolut", 2);

  olive::gl::Blit(pipeline, flipped, matrix);

  pipeline->release();

  xf->glActiveTexture(GL_TEXTURE2);
  xf->glBindTexture(GL_TEXTURE_3D, 0);
  xf->glActiveTexture(GL_TEXTURE0);
}
