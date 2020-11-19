/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "openglrenderer.h"

#include <QDebug>
#include <QOpenGLExtraFunctions>

namespace olive {

const QVector<GLfloat> blit_vertices = {
  -1.0f, -1.0f, 0.0f,
  1.0f, -1.0f, 0.0f,
  1.0f, 1.0f, 0.0f,

  -1.0f, -1.0f, 0.0f,
  -1.0f, 1.0f, 0.0f,
  1.0f, 1.0f, 0.0f
};

const QVector<GLfloat> blit_texcoords = {
  0.0f, 0.0f,
  1.0f, 0.0f,
  1.0f, 1.0f,

  0.0f, 0.0f,
  0.0f, 1.0f,
  1.0f, 1.0f
};

OpenGLRenderer::OpenGLRenderer(QObject* parent) :
  Renderer(parent),
  context_(nullptr)
{
}

OpenGLRenderer::~OpenGLRenderer()
{
  Destroy();
}

void OpenGLRenderer::Init(QOpenGLContext *existing_ctx)
{
  if (context_) {
    qCritical() << "Can't initialize already initialized OpenGLRenderer";
    return;
  }

  context_ = existing_ctx;
}

bool OpenGLRenderer::Init()
{
  if (context_) {
    qCritical() << "Can't initialize already initialized OpenGLRenderer";
    return false;
  }

  surface_.create();

  context_ = new QOpenGLContext(this);
  if (!context_->create()) {
    qCritical() << "Failed to create OpenGL context";
    return false;
  }

  context_->moveToThread(this->thread());

  return true;
}

void OpenGLRenderer::PostInit()
{
  // Make context current on that surface
  if (context_->parent() == this && !context_->makeCurrent(&surface_)) {
    qCritical() << "Failed to makeCurrent() on offscreen surface in thread" << thread();
    return;
  }

  functions_ = context_->functions();

  // Store OpenGL functions instance
  functions_->glBlendFunc(GL_ONE, GL_ZERO);

  // Set up framebuffer used for various things
  functions_->glGenFramebuffers(1, &framebuffer_);
}

void OpenGLRenderer::DestroyInternal()
{
  if (context_) {
    // Delete framebuffer
    functions_->glDeleteFramebuffers(1, &framebuffer_);

    // Delete context if it belongs to us
    if (context_->parent() == this) {
      delete context_;
    }
    context_ = nullptr;

    // Destroy surface if we created it
    if (surface_.isValid()) {
      surface_.destroy();
    }
  }
}

void OpenGLRenderer::ClearDestination(double r, double g, double b, double a)
{
  functions_->glClearColor(r, g, b, a);
  functions_->glClear(GL_COLOR_BUFFER_BIT);
}

QVariant OpenGLRenderer::CreateNativeTexture2D(int width, int height, VideoParams::Format format, int channel_count, const void *data, int linesize)
{
  GLuint texture;
  functions_->glGenTextures(1, &texture);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize);

  GLint current_tex;
  functions_->glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_tex);

  functions_->glBindTexture(GL_TEXTURE_2D, texture);

  functions_->glTexImage2D(GL_TEXTURE_2D, 0, GetInternalFormat(format, channel_count),
                           width, height, 0, GetPixelFormat(channel_count),
                           GetPixelType(format), data);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  functions_->glBindTexture(GL_TEXTURE_2D, current_tex);

  return texture;
}

QVariant OpenGLRenderer::CreateNativeTexture3D(int width, int height, int depth, VideoParams::Format format, int channel_count, const void *data, int linesize)
{
  GLuint texture;
  functions_->glGenTextures(1, &texture);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize);

  GLint current_tex;
  functions_->glGetIntegerv(GL_TEXTURE_BINDING_3D, &current_tex);

  functions_->glBindTexture(GL_TEXTURE_3D, texture);

  context_->extraFunctions()->glTexImage3D(GL_TEXTURE_3D, 0, GetInternalFormat(format, channel_count),
                                           width, height, depth, 0, GetPixelFormat(channel_count),
                                           GetPixelType(format), data);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  functions_->glBindTexture(GL_TEXTURE_3D, current_tex);

  return texture;
}

void OpenGLRenderer::AttachTextureAsDestination(Texture* texture)
{
  functions_->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  functions_->glFramebufferTexture2D(GL_FRAMEBUFFER,
                                     GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D,
                                     texture->id().value<GLuint>(),
                                     0);
}

void OpenGLRenderer::DetachTextureAsDestination()
{
  functions_->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::DestroyNativeTexture(QVariant texture)
{
  GLuint t = texture.value<GLuint>();
  functions_->glDeleteTextures(1, &t);
}

QVariant OpenGLRenderer::CreateNativeShader(ShaderCode code)
{
  QOpenGLShaderProgram* program = new QOpenGLShaderProgram(context_);

  QString vert_code = code.vert_code();
  QString frag_code = code.frag_code();

  const QString shader_preamble = QStringLiteral("#version 300 es\n"
                                                 "\n"
                                                 "#ifdef GL_ES\n"
                                                 "precision highp int;\n"
                                                 "precision highp float;\n"
                                                 "#endif\n"
                                                 "\n");

  vert_code.prepend(shader_preamble);
  frag_code.prepend(shader_preamble);

  if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, vert_code)) {
    qCritical() << "Failed to add vertex code to shader";
    goto error;
  }

  if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment, frag_code)) {
    qCritical() << "Failed to add fragment code to shader";
    goto error;
  }

  if (!program->link()) {
    qCritical() << "Failed to link shader";
    goto error;
  }

  return Node::PtrToValue(program);

error:
  delete program;
  return QVariant();
}

void OpenGLRenderer::DestroyNativeShader(QVariant shader)
{
  delete Node::ValueToPtr<QOpenGLShaderProgram>(shader);
}

void OpenGLRenderer::UploadToTexture(Texture *texture, const void *data, int linesize)
{
  GLuint t = texture->id().value<GLuint>();
  const VideoParams& p = texture->params();

  // Store currently bound texture so it can be restored later
  GLint current_tex;
  functions_->glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_tex);

  functions_->glBindTexture(GL_TEXTURE_2D, t);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize);

  functions_->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                              p.effective_width(), p.effective_height(),
                              GetPixelFormat(p.channel_count()), GetPixelType(p.format()),
                              data);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  functions_->glBindTexture(GL_TEXTURE_2D, current_tex);
}

void OpenGLRenderer::DownloadFromTexture(Texture* texture, void *data, int linesize)
{
  const VideoParams& p = texture->params();

  GLint current_tex;
  functions_->glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_tex);

  AttachTextureAsDestination(texture);

  functions_->glPixelStorei(GL_PACK_ROW_LENGTH, linesize);

  functions_->glReadPixels(0,
                           0,
                           p.width(),
                           p.height(),
                           GetPixelFormat(p.channel_count()),
                           GetPixelType(p.format()),
                           data);

  functions_->glPixelStorei(GL_PACK_ROW_LENGTH, 0);

  DetachTextureAsDestination();

  functions_->glBindTexture(GL_TEXTURE_2D, current_tex);
}

struct TextureToBind {
  TexturePtr texture;
  Texture::Interpolation interpolation;
};

void OpenGLRenderer::Blit(QVariant s, ShaderJob job, Texture *destination, VideoParams destination_params)
{
  // If this node is iterative, we'll pick up which input here
  QString iterative_name;
  GLuint iterative_input = 0;
  QVector<TextureToBind> textures_to_bind;

  QOpenGLShaderProgram* shader = Node::ValueToPtr<QOpenGLShaderProgram>(s);

  shader->bind();

  NodeValueMap::const_iterator it;
  for (it=job.GetValues().constBegin(); it!=job.GetValues().constEnd(); it++) {
    // See if the shader has takes this parameter as an input
    int variable_location = shader->uniformLocation(it.key());

    if (variable_location == -1) {
      continue;
    }

    // This variable is used in the shader, let's set it
    const ShaderValue& value = it.value();

    if (value.array) {
      qWarning() << "FIXME: Array support is currently a stub";
    }

    switch (value.type) {
    case NodeInput::kInt:
      // kInt technically specifies a LongLong, but OpenGL doesn't support those. This may lead to
      // over/underflows if the number is large enough, but the likelihood of that is quite low.
      shader->setUniformValue(variable_location, value.data.toInt());
      break;
    case NodeInput::kFloat:
      // kFloat technically specifies a double but as above, OpenGL doesn't support those.
      shader->setUniformValue(variable_location, value.data.toFloat());
      break;
    case NodeInput::kVec2:
      shader->setUniformValue(variable_location, value.data.value<QVector2D>());
      break;
    case NodeInput::kVec3:
      shader->setUniformValue(variable_location, value.data.value<QVector3D>());
      break;
    case NodeInput::kVec4:
      shader->setUniformValue(variable_location, value.data.value<QVector4D>());
      break;
    case NodeInput::kMatrix:
      shader->setUniformValue(variable_location, value.data.value<QMatrix4x4>());
      break;
    case NodeInput::kCombo:
      shader->setUniformValue(variable_location, value.data.value<int>());
      break;
    case NodeInput::kColor:
    {
      Color color = value.data.value<Color>();
      shader->setUniformValue(variable_location,
                              color.red(), color.green(), color.blue(), color.alpha());
      break;
    }
    case NodeInput::kBoolean:
      shader->setUniformValue(variable_location, value.data.toBool());
      break;
    case NodeInput::kBuffer:
    case NodeInput::kTexture:
    {
      TexturePtr texture = value.data.value<TexturePtr>();

      // Set value to bound texture
      shader->setUniformValue(variable_location, textures_to_bind.size());

      // If this texture binding is the iterative input, set it here
      if (it.key() == job.GetIterativeInput()) {
        iterative_input = textures_to_bind.size();
        iterative_name = it.key();
      }

      GLuint tex_id = texture ? texture->id().value<GLuint>() : 0;
      textures_to_bind.append({texture, job.GetInterpolation(it.key())});

      // Set enable flag if shader wants it
      int enable_param_location = shader->uniformLocation(QStringLiteral("%1_enabled").arg(it.key()));
      if (enable_param_location > -1) {
        shader->setUniformValue(enable_param_location,
                                tex_id > 0);
      }

      if (tex_id > 0) {
        // Set texture resolution if shader wants it
        int res_param_location = shader->uniformLocation(QStringLiteral("%1_resolution").arg(it.key()));
        if (res_param_location > -1) {
          int virtual_width = texture->params().width();

          // Adjust virtual width by pixel aspect if necessary
          if (texture->params().pixel_aspect_ratio() != 1
              || destination_params.pixel_aspect_ratio() != 1) {
            double relative_pixel_aspect = texture->params().pixel_aspect_ratio().toDouble() / destination_params.pixel_aspect_ratio().toDouble();

            virtual_width = qRound(static_cast<double>(virtual_width) * relative_pixel_aspect);
          }

          shader->setUniformValue(res_param_location,
                                  virtual_width,
                                  static_cast<GLfloat>(texture->height() * texture->divider()));
        }
      }
      break;
    }
    case NodeInput::kSamples:
    case NodeInput::kText:
    case NodeInput::kRational:
    case NodeInput::kFont:
    case NodeInput::kFile:
    case NodeInput::kDecimal:
    case NodeInput::kNumber:
    case NodeInput::kString:
    case NodeInput::kVector:
    case NodeInput::kShaderJob:
    case NodeInput::kSampleJob:
    case NodeInput::kGenerateJob:
    case NodeInput::kFootage:
    case NodeInput::kNone:
    case NodeInput::kAny:
      break;
    }
  }

  // Bind all textures
  for (int i=0; i<textures_to_bind.size(); i++) {
    const TextureToBind& t = textures_to_bind.at(i);
    TexturePtr texture = t.texture;

    GLuint tex_id = texture ? texture->id().value<GLuint>() : 0;

    functions_->glActiveTexture(GL_TEXTURE0 + i);

    GLenum target = (texture->type() == Texture::k3D) ? GL_TEXTURE_3D : GL_TEXTURE_2D;
    functions_->glBindTexture(target, tex_id);

    PrepareInputTexture(target, t.interpolation);
  }

  // Set ove_resolution to the destination to the "logical" resolution of the destination
  shader->setUniformValue("ove_resolution",
                          static_cast<GLfloat>(destination_params.width()),
                          static_cast<GLfloat>(destination_params.height()));

  // Ensure matrix is set, at least to identity
  shader->setUniformValue("ove_mvpmat",
                          job.GetValue(QStringLiteral("ove_mvpmat")).data.value<QMatrix4x4>());

  // Set the viewport to the "physical" resolution of the destination
  functions_->glViewport(0, 0,
                         destination_params.effective_width(),
                         destination_params.effective_height());

  // Bind vertex array object
  QOpenGLVertexArrayObject vao_;
  vao_.create();
  vao_.bind();

  // Set buffers
  QOpenGLBuffer vert_vbo_;
  vert_vbo_.create();
  vert_vbo_.bind();
  vert_vbo_.allocate(blit_vertices.constData(), blit_vertices.size() * sizeof(GLfloat));
  vert_vbo_.release();

  QOpenGLBuffer frag_vbo_;
  frag_vbo_.create();
  frag_vbo_.bind();
  frag_vbo_.allocate(blit_texcoords.constData(), blit_texcoords.size() * sizeof(GLfloat));
  frag_vbo_.release();

  int vertex_location = shader->attributeLocation("a_position");
  vert_vbo_.bind();
  functions_->glEnableVertexAttribArray(vertex_location);
  functions_->glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  vert_vbo_.release();

  int tex_location = shader->attributeLocation("a_texcoord");
  frag_vbo_.bind();
  functions_->glEnableVertexAttribArray(tex_location);
  functions_->glVertexAttribPointer(tex_location, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  frag_vbo_.release();

  // Some shaders optimize through multiple iterations which requires ping-ponging textures
  // - If there are only two iterations, we can just create one backend texture and then the
  //   destination can be the second
  // - If there are more than two iterations, we need to ping pong back and forth between two
  //   textures. We can still use the destination as the last iteration, but we'll need textures
  //   for the iterative process.
  int real_iteration_count;
  if (job.GetIterationCount() > 1 && !job.GetIterativeInput().isEmpty()) {
    real_iteration_count = job.GetIterationCount();
  } else {
    real_iteration_count = 1;
  }

  TexturePtr output_tex, input_tex;
  if (real_iteration_count > 1) {
    // Create one texture to bounce off
    output_tex = CreateTexture(destination_params);

    if (real_iteration_count > 2) {
      // Create a second texture bounce off
      input_tex = CreateTexture(destination_params);
    }
  }

  for (int iteration=0; iteration<real_iteration_count; iteration++) {
    // Set iteration number
    shader->setUniformValue("ove_iteration", iteration);

    // Replace iterative input
    if (iteration == real_iteration_count-1) {
      // This is the last iteration, draw to the destination
      if (destination) {
        // If we have a destination texture, draw to it
        AttachTextureAsDestination(destination);
      } else if (iteration > 0) {
        // Otherwise, if we were iterating before, detach texture now
        DetachTextureAsDestination();
      }

      // Clear the destination, whatever it is
      ClearDestination();
    } else {
      // Always draw to output_tex, which gets swapped with input_tex every iteration
      AttachTextureAsDestination(output_tex.get());
    }

    if (iteration > 0) {
      // If this is not the first iteration, replace the iterative texture with the one we
      // last drew
      functions_->glActiveTexture(GL_TEXTURE0 + iterative_input);
      functions_->glBindTexture(GL_TEXTURE_2D, input_tex->id().value<GLuint>());

      // At this time, we only support iterating 2D textures
      PrepareInputTexture(GL_TEXTURE_2D, job.GetInterpolation(iterative_name));
    }

    // Swap so that the next iteration, the texture we draw now will be the input texture next
    std::swap(output_tex, input_tex);

    // Blit this texture through this shader
    functions_->glDrawArrays(GL_TRIANGLES, 0, blit_vertices.size() / 3);
  }

  if (destination) {
    // Reset framebuffer to default if we were drawing to a texture
    DetachTextureAsDestination();
  }

  // Release any textures we bound before
  for (int i=textures_to_bind.size()-1; i>=0; i--) {
    GLenum target = (textures_to_bind.at(i).texture->type() == Texture::k3D) ? GL_TEXTURE_3D : GL_TEXTURE_2D;
    functions_->glActiveTexture(GL_TEXTURE0 + i);
    functions_->glBindTexture(target, 0);
  }

  // Release shader
  shader->release();

  // Release vertex array object
  frag_vbo_.destroy();
  vert_vbo_.destroy();
  vao_.release();
  vao_.destroy();
}

GLint OpenGLRenderer::GetInternalFormat(VideoParams::Format format, int channel_layout)
{
  switch (format) {
  case VideoParams::kFormatUnsigned8:
    switch (channel_layout) {
    case 1:
      return GL_R8;
    case 2:
      return GL_RG8;
    case 3:
      return GL_RGB8;
    case 4:
      return GL_RGBA8;
    }
    break;
  case VideoParams::kFormatUnsigned16:
    switch (channel_layout) {
    case 1:
      return GL_R16;
    case 2:
      return GL_RG16;
    case 3:
      return GL_RGB16;
    case 4:
      return GL_RGBA16;
    }
    break;
  case VideoParams::kFormatFloat16:
    switch (channel_layout) {
    case 1:
      return GL_R16F;
    case 2:
      return GL_RG16F;
    case 3:
      return GL_RGB16F;
    case 4:
      return GL_RGBA16F;
    }
    break;
  case VideoParams::kFormatFloat32:
    switch (channel_layout) {
    case 1:
      return GL_R32F;
    case 2:
      return GL_RG32F;
    case 3:
      return GL_RGB32F;
    case 4:
      return GL_RGBA32F;
    }
    break;
  case VideoParams::kFormatInvalid:
  case VideoParams::kFormatCount:
    break;
  }

  return GL_INVALID_VALUE;
}

GLenum OpenGLRenderer::GetPixelType(VideoParams::Format format)
{
  switch (format) {
  case VideoParams::kFormatUnsigned8:
    return GL_UNSIGNED_BYTE;
  case VideoParams::kFormatUnsigned16:
    return GL_UNSIGNED_SHORT;
  case VideoParams::kFormatFloat16:
    return GL_HALF_FLOAT;
  case VideoParams::kFormatFloat32:
    return GL_FLOAT;

  case VideoParams::kFormatInvalid:
  case VideoParams::kFormatCount:
    break;
  }

  return GL_INVALID_VALUE;
}

GLenum OpenGLRenderer::GetPixelFormat(int channel_count)
{
  switch (channel_count) {
  case 1:
    return GL_RED;
  case 3:
    return GL_RGB;
  case 4:
    return GL_RGBA;
  default:
    return GL_INVALID_VALUE;
  }
}

void OpenGLRenderer::PrepareInputTexture(GLenum target, Texture::Interpolation interp)
{
  switch (interp) {
  case Texture::kNearest:
    functions_->glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    functions_->glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    break;
  case Texture::kLinear:
    functions_->glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    functions_->glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    break;
  case Texture::kMipmappedLinear:
    functions_->glGenerateMipmap(target);
    functions_->glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    functions_->glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    break;
  }

  functions_->glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  functions_->glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

}
