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

#include "openglrenderer.h"

#include <iostream>
#include <QDateTime>
#include <QDebug>
#include <QOpenGLExtraFunctions>

namespace olive {

const int OpenGLRenderer::kTextureCacheMaxSize = 5000;

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

class ErrorPrinter {
public:
  ErrorPrinter(const char* name, QOpenGLFunctions* f)
  {
    GLuint err = f->glGetError();
    if (err > 0)
      qDebug() << name << "entered with" << err;

    name_ = name;
    functions_ = f;
  }

  ~ErrorPrinter()
  {
    GLuint err = functions_->glGetError();
    if (err > 0)
      qDebug() << name_ << "exited with" << err;
  }

private:
  const char* name_;

  QOpenGLFunctions* functions_;

};

#define PRINT_GL_ERRORS ErrorPrinter __e(__FUNCTION__, functions_)

#define GL_PREAMBLE \
  //QMutexLocker __l(&global_opengl_mutex);

//QMutex global_opengl_mutex;

OpenGLRenderer::OpenGLRenderer(QObject* parent) :
  Renderer(parent),
  context_(nullptr),
  framebuffer_(0)
{
}

OpenGLRenderer::~OpenGLRenderer()
{
  Destroy();
  PostDestroy();
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
  GL_PREAMBLE;

  if (context_) {
    qCritical() << "Can't initialize already initialized OpenGLRenderer";
    return false;
  }

  surface_.create();

  context_ = new QOpenGLContext(this);
  context_->setShareContext(QOpenGLContext::globalShareContext());
  if (!context_->create()) {
    qCritical() << "Failed to create OpenGL context";
    return false;
  }

  context_->moveToThread(this->thread());

  return true;
}

void OpenGLRenderer::PostDestroy()
{
  // Destroy surface if we created it
  if (surface_.isValid()) {
    surface_.destroy();
  }
}

void OpenGLRenderer::PostInit()
{
  GL_PREAMBLE;

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
    GL_PREAMBLE;

    // Delete framebuffer
    functions_->glDeleteFramebuffers(1, &framebuffer_);
    framebuffer_ = 0;

    // Delete context if it belongs to us
    if (context_->parent() == this) {
      delete context_;
    }
    context_ = nullptr;
  }
}

void OpenGLRenderer::ClearDestination(Texture *texture, double r, double g, double b, double a)
{
  GL_PREAMBLE;

  if (texture) {
    AttachTextureAsDestination(texture->id());
  }

  ClearDestinationInternal(r, g, b, a);

  if (texture) {
    DetachTextureAsDestination();
  }
}

QVariant OpenGLRenderer::CreateNativeTexture(int width, int height, int depth, VideoParams::Format format, int channel_count, const void *data, int linesize)
{
  GL_PREAMBLE;

  bool is_3d = depth > 1;

  // Generate new texture
  GLuint texture;
  functions_->glGenTextures(1, &texture);
  texture_params_.insert(texture, {width, height, depth, format, channel_count});

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize);

  GLenum target_current = is_3d ? GL_TEXTURE_BINDING_3D : GL_TEXTURE_BINDING_2D;
  GLenum target = is_3d ? GL_TEXTURE_3D : GL_TEXTURE_2D;

  GLint current_tex;
  functions_->glGetIntegerv(target_current, &current_tex);

  functions_->glBindTexture(target, texture);

  if (is_3d) {
    context_->extraFunctions()->glTexImage3D(target, 0, GetInternalFormat(format, channel_count),
                                             width, height, depth, 0, GetPixelFormat(channel_count),
                                             GetPixelType(format), data);
  } else {
    functions_->glTexImage2D(target, 0, GetInternalFormat(format, channel_count),
                             width, height, 0, GetPixelFormat(channel_count),
                             GetPixelType(format), data);
  }

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  functions_->glBindTexture(target, current_tex);

  return texture;
}

void OpenGLRenderer::AttachTextureAsDestination(const QVariant &texture)
{
  PRINT_GL_ERRORS;

  functions_->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  functions_->glFramebufferTexture2D(GL_FRAMEBUFFER,
                                     GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D,
                                     texture.value<GLuint>(),
                                     0);
}

void OpenGLRenderer::DetachTextureAsDestination()
{
  functions_->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::DestroyNativeTexture(QVariant texture)
{
  GLuint t = texture.value<GLuint>();

  if (t > 0) {
    functions_->glDeleteTextures(1, &t);
  }
}

QVariant OpenGLRenderer::CreateNativeShader(ShaderCode code)
{
  GL_PREAMBLE;

  PRINT_GL_ERRORS;

  GLuint vert = CompileShader(GL_VERTEX_SHADER, code.vert_code());
  GLuint frag = CompileShader(GL_FRAGMENT_SHADER, code.frag_code());

  GLuint program = 0;

  if (frag && vert) {
    program = functions_->glCreateProgram();
    functions_->glAttachShader(program, frag);
    functions_->glAttachShader(program, vert);
    functions_->glLinkProgram(program);

    GLint success;
    functions_->glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
      qWarning() << "Failed to link OpenGL shader program";
      functions_->glDeleteProgram(program);
      program = 0;
    }
  }

  functions_->glDeleteShader(frag);
  functions_->glDeleteShader(vert);

  return program;
}

void OpenGLRenderer::DestroyNativeShader(QVariant shader)
{
  GL_PREAMBLE;

  GLuint program = shader.value<GLuint>();
  functions_->glDeleteProgram(program);
}

void OpenGLRenderer::UploadToTexture(const QVariant &handle, const VideoParams &p, const void *data, int linesize)
{
  GL_PREAMBLE;

  GLuint t = handle.value<GLuint>();

  bool is_3d = p.is_3d();

  GLenum tex_type = !is_3d ? GL_TEXTURE_2D : GL_TEXTURE_3D;
  GLenum tex_binding = !is_3d ? GL_TEXTURE_BINDING_2D : GL_TEXTURE_BINDING_3D;

  // Store currently bound texture so it can be restored later
  GLint current_tex;
  functions_->glGetIntegerv(tex_binding, &current_tex);

  functions_->glBindTexture(tex_type, t);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize);

  {
    PRINT_GL_ERRORS;

    if (!is_3d) {
      functions_->glTexSubImage2D(tex_type, 0, 0, 0,
                                  p.effective_width(), p.effective_height(),
                                  GetPixelFormat(p.channel_count()), GetPixelType(p.format()),
                                  data);
    } else {
      context_->extraFunctions()->glTexSubImage3D(tex_type, 0, 0, 0, 0,
                                                  p.effective_width(), p.effective_height(), p.effective_depth(),
                                                  GetPixelFormat(p.channel_count()), GetPixelType(p.format()),
                                                  data);
    }
  }

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  functions_->glBindTexture(tex_type, current_tex);
}

void OpenGLRenderer::DownloadFromTexture(const QVariant &id, const VideoParams &p, void *data, int linesize)
{
  GL_PREAMBLE;

  GLint current_tex;
  functions_->glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_tex);

  AttachTextureAsDestination(id);

  functions_->glPixelStorei(GL_PACK_ROW_LENGTH, linesize);

  {
    PRINT_GL_ERRORS;
    functions_->glReadPixels(0,
                             0,
                             p.effective_width(),
                             p.effective_height(),
                             GetPixelFormat(p.channel_count()),
                             GetPixelType(p.format()),
                             data);
  }

  functions_->glPixelStorei(GL_PACK_ROW_LENGTH, 0);

  DetachTextureAsDestination();

  functions_->glBindTexture(GL_TEXTURE_2D, current_tex);
}

void OpenGLRenderer::Flush()
{
  GL_PREAMBLE;

  functions_->glFinish();
}

Color OpenGLRenderer::GetPixelFromTexture(Texture *texture, const QPointF &pt)
{
  AttachTextureAsDestination(texture->id());

  QByteArray data(VideoParams::GetBytesPerPixel(texture->format(), texture->channel_count()), Qt::Uninitialized);

  functions_->glReadPixels(pt.x(), pt.y(), 1, 1, GetPixelFormat(texture->channel_count()), GetPixelType(texture->format()), data.data());

  Color c = Color::fromData(data.data(), texture->format(), texture->channel_count());

  if (texture->channel_count() == VideoParams::kRGBChannelCount) {
    // No alpha channel, set to 1.0
    c.set_alpha(1.0);
  }

  DetachTextureAsDestination();

  return c;
}

struct TextureToBind {
  TexturePtr texture;
  Texture::Interpolation interpolation;
};

void OpenGLRenderer::Blit(QVariant s, ShaderJob job, Texture *destination, VideoParams destination_params, bool clear_destination)
{
  GL_PREAMBLE;

  // If this node is iterative, we'll pick up which input here
  QMap<QString, GLuint> texture_index_map;
  QVector<TextureToBind> textures_to_bind;

  GLuint shader = s.value<GLuint>();

  functions_->glUseProgram(shader);

  for (auto it=job.GetValues().constBegin(); it!=job.GetValues().constEnd(); it++) {
    // See if the shader has takes this parameter as an input
    GLint variable_location = functions_->glGetUniformLocation(shader, it.key().toUtf8().constData());

    if (variable_location == -1) {
      continue;
    }

    // This variable is used in the shader, let's set it
    const NodeValue& value = it.value();

    if (value.array()) {
      continue;
    }

    switch (value.type()) {
    case NodeValue::kInt:
      // kInt technically specifies a LongLong, but OpenGL doesn't support those. This may lead to
      // over/underflows if the number is large enough, but the likelihood of that is quite low.
      functions_->glUniform1i(variable_location, value.toInt());
      break;
    case NodeValue::kFloat:
      // kFloat technically specifies a double but as above, OpenGL doesn't support those.
      functions_->glUniform1f(variable_location, value.toDouble());
      break;
    case NodeValue::kVec2:
    {
      QVector2D v = value.toVec2();
      functions_->glUniform2fv(variable_location, 1, reinterpret_cast<const GLfloat*>(&v));
      break;
    }
    case NodeValue::kVec3:
    {
      QVector3D v = value.toVec3();
      functions_->glUniform3fv(variable_location, 1, reinterpret_cast<const GLfloat*>(&v));
      break;
    }
    case NodeValue::kVec4:
    {
      QVector4D v = value.toVec4();
      functions_->glUniform4fv(variable_location, 1, reinterpret_cast<const GLfloat*>(&v));
      break;
    }
    case NodeValue::kMatrix:
      functions_->glUniformMatrix4fv(variable_location, 1, false, value.toMatrix().constData());
      break;
    case NodeValue::kCombo:
      functions_->glUniform1i(variable_location, value.toInt());
      break;
    case NodeValue::kColor:
    {
      Color color = value.toColor();
      functions_->glUniform4f(variable_location, color.red(), color.green(), color.blue(), color.alpha());
      break;
    }
    case NodeValue::kBoolean:
      functions_->glUniform1i(variable_location, value.toBool());
      break;
    case NodeValue::kTexture:
    {
      TexturePtr texture = value.toTexture();

      // Set value to bound texture
      functions_->glUniform1i(variable_location, textures_to_bind.size());

      texture_index_map.insert(it.key(), textures_to_bind.size());

      textures_to_bind.append({texture, job.GetInterpolation(it.key())});

      // Set enable flag if shader wants it
      GLuint tex_id = texture ? texture->id().value<GLuint>() : 0;
      int enable_param_location = functions_->glGetUniformLocation(shader, QStringLiteral("%1_enabled").arg(it.key()).toUtf8().constData());
      if (enable_param_location > -1) {
        functions_->glUniform1i(enable_param_location, tex_id > 0);
      }
      break;
    }
    case NodeValue::kSamples:
    case NodeValue::kText:
    case NodeValue::kRational:
    case NodeValue::kFont:
    case NodeValue::kFile:
    case NodeValue::kVideoParams:
    case NodeValue::kAudioParams:
    case NodeValue::kSubtitleParams:
    case NodeValue::kBezier:
    case NodeValue::kNone:
    case NodeValue::kDataTypeCount:
      break;
    }
  }

  // Bind all textures
  for (int i=0; i<textures_to_bind.size(); i++) {
    const TextureToBind& t = textures_to_bind.at(i);
    TexturePtr texture = t.texture;

    GLuint tex_id = texture ? texture->id().value<GLuint>() : 0;

    functions_->glActiveTexture(GL_TEXTURE0 + i);

    GLenum target = (texture && texture->params().is_3d()) ? GL_TEXTURE_3D : GL_TEXTURE_2D;
    functions_->glBindTexture(target, tex_id);

    if (tex_id) {
      PrepareInputTexture(target, t.interpolation);

      if (texture->channel_count() == 1 && destination_params.channel_count() != 1) {
        // Interpret this texture as a grayscale texture
        functions_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
        functions_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
        functions_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
      }
    }
  }

  // Ensure matrix is set, at least to identity
  GLint mvpmat_location = functions_->glGetUniformLocation(shader, "ove_mvpmat");
  if (mvpmat_location > -1) {
    functions_->glUniformMatrix4fv(mvpmat_location, 1, false, job.Get(QStringLiteral("ove_mvpmat")).toMatrix().constData());
  }

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
  // If the job has vertex coordinate overrides use them instead of the defaults.
  if (!job.GetVertexCoordinates().isEmpty()) {
    Q_ASSERT(job.GetVertexCoordinates().size() == 18);
    vert_vbo_.allocate(job.GetVertexCoordinates().constData(), job.GetVertexCoordinates().size() * sizeof(float));
  } else {
    vert_vbo_.allocate(blit_vertices.constData(), blit_vertices.size() * sizeof(GLfloat));
  }
  vert_vbo_.release();

  QOpenGLBuffer frag_vbo_;
  frag_vbo_.create();
  frag_vbo_.bind();
  frag_vbo_.allocate(blit_texcoords.constData(), blit_texcoords.size() * sizeof(GLfloat));
  frag_vbo_.release();

  GLint vertex_location = functions_->glGetAttribLocation(shader, "a_position");
  if (vertex_location != -1) {
    vert_vbo_.bind();
    functions_->glEnableVertexAttribArray(vertex_location);
    functions_->glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    vert_vbo_.release();
  }

  GLint tex_location = functions_->glGetAttribLocation(shader, "a_texcoord");
  if (tex_location != -1) {
    frag_vbo_.bind();
    functions_->glEnableVertexAttribArray(tex_location);
    functions_->glVertexAttribPointer(tex_location, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    frag_vbo_.release();
  }

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

  GLint iteration_location = functions_->glGetUniformLocation(shader, "ove_iteration");
  for (int iteration=0; iteration<real_iteration_count; iteration++) {
    // Set iteration number
    if (iteration_location > -1) {
      functions_->glUniform1i(iteration_location, iteration);
    }

    // Replace iterative input
    if (iteration == real_iteration_count-1) {
      // This is the last iteration, draw to the destination
      if (destination) {
        // If we have a destination texture, draw to it
        AttachTextureAsDestination(destination->id());
      } else if (iteration > 0) {
        // Otherwise, if we were iterating before, detach texture now
        DetachTextureAsDestination();
      }

      // Clear the destination if the caller requested it
      if (clear_destination) {
        ClearDestinationInternal();
      }
    } else {
      // Always draw to output_tex, which gets swapped with input_tex every iteration
      AttachTextureAsDestination(output_tex->id());
    }

    if (iteration > 0) {
      // If this is not the first iteration, replace the iterative texture with the one we
      // last drew
      const QString &iterative_input = job.GetIterativeInput();
      functions_->glActiveTexture(GL_TEXTURE0 + texture_index_map.value(iterative_input));
      functions_->glBindTexture(GL_TEXTURE_2D, input_tex->id().value<GLuint>());

      // At this time, we only support iterating 2D textures
      PrepareInputTexture(GL_TEXTURE_2D, job.GetInterpolation(iterative_input));
    }

    // Swap so that the next iteration, the texture we draw now will be the input texture next
    std::swap(output_tex, input_tex);

    // Blit this texture through this shader
    {
      PRINT_GL_ERRORS;
      functions_->glDrawArrays(GL_TRIANGLES, 0, blit_vertices.size() / 3);
    }
  }

  if (destination) {
    // Reset framebuffer to default if we were drawing to a texture
    DetachTextureAsDestination();
  }

  // Release any textures we bound before
  for (int i=textures_to_bind.size()-1; i>=0; i--) {
    TexturePtr texture = textures_to_bind.at(i).texture;
    GLenum target = (texture && texture->params().is_3d()) ? GL_TEXTURE_3D : GL_TEXTURE_2D;
    functions_->glActiveTexture(GL_TEXTURE0 + i);
    functions_->glBindTexture(target, 0);
  }

  // Release shader
  functions_->glUseProgram(0);

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

  if (target == GL_TEXTURE_3D) {
    functions_->glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  }
}

void OpenGLRenderer::ClearDestinationInternal(double r, double g, double b, double a)
{
  functions_->glClearColor(r, g, b, a);
  functions_->glClear(GL_COLOR_BUFFER_BIT);
}

GLuint OpenGLRenderer::CompileShader(GLenum type, const QString &code)
{
  static const QString shader_preamble =
      // Use appropriate GL 3.2 shader header
      QStringLiteral("#version 150\n\n"
                     "precision highp float;\n\n");

  QString complete_code;

  if (!code.startsWith(QStringLiteral("#version"))) {
    complete_code = shader_preamble;
  }

  if (code.isEmpty()) {
    // Use default code
    if (type == GL_FRAGMENT_SHADER) {
      complete_code.append(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/default.frag")));
    } else if (type == GL_VERTEX_SHADER) {
      complete_code.append(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/default.vert")));
    }
  } else {
    complete_code.append(code);
  }

  QByteArray code_utf8 = complete_code.toUtf8();
  const char *code_cstr = code_utf8.constData();

  GLuint shader = functions_->glCreateShader(type);
  functions_->glShaderSource(shader, 1, &code_cstr, nullptr);
  functions_->glCompileShader(shader);

  GLint success;
  functions_->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    qWarning() << "Failed to compile OpenGL shader";

    QByteArray error_log(10240, Qt::Uninitialized);
    functions_->glGetShaderInfoLog(shader, error_log.size(), nullptr, error_log.data());
    std::cout << error_log.constData() << std::endl << code_cstr << std::endl;

    functions_->glDeleteShader(shader);
    shader = 0;
  }

  return shader;
}

}
