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

#include "openglrenderer.h"

#include <QDebug>

OLIVE_NAMESPACE_ENTER

OpenGLRenderer::OpenGLRenderer(QObject* parent) :
  Renderer(parent),
  context_(nullptr)
{
}

OpenGLRenderer::~OpenGLRenderer()
{
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
  if (!context_->makeCurrent(&surface_)) {
    qCritical() << "Failed to makeCurrent() on offscreen surface in thread" << thread();
    return;
  }

  // Store OpenGL functions instance
  functions_ = context_->functions();
  functions_->glBlendFunc(GL_ONE, GL_ZERO);
}

void OpenGLRenderer::Destroy()
{
  if (context_->parent() == this) {
    delete context_;
  }
  context_ = nullptr;

  qDeleteAll(shader_cache_);
  shader_cache_.clear();

  if (surface_.isValid()) {
    surface_.destroy();
  }
}

QVariant OpenGLRenderer::CreateNativeTexture(const VideoParams &p, void *data, int linesize)
{
  GLuint texture;
  functions_->glGenTextures(1, &texture);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize);

  functions_->glTexImage2D(GL_TEXTURE_2D, 0, GetInternalFormat(p.format()),
                           p.width(), p.height(), 0, GetPixelFormat(p.format()),
                           GetPixelType(p.format()), data);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  return texture;
}

void OpenGLRenderer::DestroyNativeTexture(QVariant texture)
{
  GLuint t = texture.value<GLuint>();
  functions_->glDeleteTextures(1, &t);
}

void OpenGLRenderer::UploadToTexture(Texture *texture, void *data, int linesize)
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
                              GetPixelFormat(p.format()), GetPixelType(p.format()),
                              data);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  functions_->glBindTexture(GL_TEXTURE_2D, current_tex);
}

void OpenGLRenderer::DownloadFromTexture(Texture* texture, void *data, int linesize)
{
  GLuint t = texture->id().value<GLuint>();
  const VideoParams& p = texture->params();

  GLint current_tex;
  functions_->glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_tex);

  functions_->glBindTexture(GL_TEXTURE_2D, t);

  functions_->glPixelStorei(GL_PACK_ROW_LENGTH, linesize);

  functions_->glReadPixels(0,
                           0,
                           p.width(),
                           p.height(),
                           GetPixelFormat(p.format()),
                           GetPixelType(p.format()),
                           data);

  functions_->glPixelStorei(GL_PACK_ROW_LENGTH, 0);

  functions_->glBindTexture(GL_TEXTURE_2D, current_tex);
}

Renderer::TexturePtr OpenGLRenderer::ProcessShader(const Node *node, const TimeRange &range, const ShaderJob &job, const VideoParams &params)
{
  // If this node is iterative, we'll pick up which input here
  GLuint iterative_input = 0;
  QList<GLuint> textures_to_bind;
  bool input_textures_have_alpha = false;

  QString full_shader_id = QStringLiteral("%1:%2").arg(node->id(), job.GetShaderID());
  QOpenGLShaderProgram* shader = shader_cache_.value(full_shader_id);

  if (!shader) {
    // Since we have shader code, compile it now
    ShaderCode code = node->GetShaderCode(job.GetShaderID());
    QString vert_code = code.vert_code();
    QString frag_code = code.frag_code();

    if (frag_code.isEmpty() && vert_code.isEmpty()) {
      qWarning() << "No shader code found for" << node->id() << "- operation will be a no-op";
    }

    if (frag_code.isEmpty()) {
      frag_code = OpenGLShader::CodeDefaultFragment();
    }

    if (vert_code.isEmpty()) {
      vert_code = OpenGLShader::CodeDefaultVertex();
    }

    shader = new QOpenGLShaderProgram(this);
    if (shader
        && shader->create()
        && shader->addShaderFromSourceCode(QOpenGLShader::Fragment, frag_code)
        && shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vert_code)
        && shader->link()) {
      shader_cache_.insert(full_shader_id, shader);
    } else {
      qWarning() << "Failed to compile shader for" << node->id();
      shader = nullptr;
    }

    if (!shader) {
      // Couldn't find or build the shader required
      return nullptr;
    }
  }

  shader->bind();

  NodeValueMap::const_iterator it;
  for (it=job.GetValues().constBegin(); it!=job.GetValues().constEnd(); it++) {
    // See if the shader has takes this parameter as an input
    int variable_location = shader->uniformLocation(it.key());

    if (variable_location == -1) {
      continue;
    }

    // See if this value corresponds to an input (NOTE: it may not and this may be null)
    NodeInput* corresponding_input = node->GetInputWithID(it.key());

    // This variable is used in the shader, let's set it
    const QVariant& value = it.value().data();

    NodeParam::DataType data_type = (it.value().type() != NodeParam::kNone)
        ? it.value().type()
        : corresponding_input->data_type();

    switch (data_type) {
    case NodeInput::kInt:
      // kInt technically specifies a LongLong, but OpenGL doesn't support those. This may lead to
      // over/underflows if the number is large enough, but the likelihood of that is quite low.
      shader->setUniformValue(variable_location, value.toInt());
      break;
    case NodeInput::kFloat:
      // kFloat technically specifies a double but as above, OpenGL doesn't support those.
      shader->setUniformValue(variable_location, value.toFloat());
      break;
    case NodeInput::kVec2:
      if (corresponding_input && corresponding_input->IsArray()) {
        QVector<NodeValue> nv = value.value< QVector<NodeValue> >();
        QVector<QVector2D> a(nv.size());

        for (int j=0;j<a.size();j++) {
          a[j] = nv.at(j).data().value<QVector2D>();
        }

        shader->setUniformValueArray(variable_location, a.constData(), a.size());

        int count_location = shader->uniformLocation(QStringLiteral("%1_count").arg(it.key()));
        if (count_location > -1) {
          shader->setUniformValue(count_location, a.size());
        }
      } else {
        shader->setUniformValue(variable_location, value.value<QVector2D>());
      }
      break;
    case NodeInput::kVec3:
      shader->setUniformValue(variable_location, value.value<QVector3D>());
      break;
    case NodeInput::kVec4:
      shader->setUniformValue(variable_location, value.value<QVector4D>());
      break;
    case NodeInput::kMatrix:
      shader->setUniformValue(variable_location, value.value<QMatrix4x4>());
      break;
    case NodeInput::kCombo:
      shader->setUniformValue(variable_location, value.value<int>());
      break;
    case NodeInput::kColor:
    {
      Color color = value.value<Color>();

      shader->setUniformValue(variable_location, color.red(), color.green(), color.blue(), color.alpha());
      break;
    }
    case NodeInput::kBoolean:
      shader->setUniformValue(variable_location, value.toBool());
      break;
    case NodeInput::kBuffer:
    case NodeInput::kTexture:
    {
      TexturePtr texture = value.value<TexturePtr>();

      if (texture) {
        if (PixelFormat::FormatHasAlphaChannel(texture->format())) {
          input_textures_have_alpha = true;
        }
      }

      // Set value to bound texture
      shader->setUniformValue(variable_location, textures_to_bind.size());

      // If this texture binding is the iterative input, set it here
      if (corresponding_input && corresponding_input == job.GetIterativeInput()) {
        iterative_input = textures_to_bind.size();
      }

      GLuint tex_id = texture ? texture->id().value<GLuint>() : 0;
      textures_to_bind.append(tex_id);

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
          int adjusted_width = texture->width() * texture->divider();

          // Adjust virtual width by pixel aspect if necessary
          if (texture->params().pixel_aspect_ratio() != 1
              || params.pixel_aspect_ratio() != 1) {
            double relative_pixel_aspect = texture->params().pixel_aspect_ratio().toDouble() / params.pixel_aspect_ratio().toDouble();

            adjusted_width = qRound(static_cast<double>(adjusted_width) * relative_pixel_aspect);
          }

          shader->setUniformValue(res_param_location,
                                  adjusted_width,
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

  // Provide some standard args
  shader->setUniformValue("ove_resolution",
                          static_cast<GLfloat>(params.width()),
                          static_cast<GLfloat>(params.height()));

  shader->release();

  // Create the output textures
  PixelFormat::Format output_format = (input_textures_have_alpha || job.GetAlphaChannelRequired())
      ? PixelFormat::GetFormatWithAlphaChannel(params.format())
      : PixelFormat::GetFormatWithoutAlphaChannel(params.format());
  VideoParams output_params(params.width(),
                            params.height(),
                            params.time_base(),
                            output_format,
                            params.pixel_aspect_ratio(),
                            params.interlacing(),
                            params.divider());

  int real_iteration_count;
  if (job.GetIterationCount() > 1 && job.GetIterativeInput()) {
    real_iteration_count = job.GetIterationCount();
  } else {
    real_iteration_count = 1;
  }

  TexturePtr dst_refs[2];
  dst_refs[0] = CreateTexture(output_params);

  // If this node requires multiple iterations, get a texture for it too
  if (real_iteration_count > 1) {
    dst_refs[1] = CreateTexture(output_params);
  }

  // Some nodes use multiple iterations for optimization
  TexturePtr input_tex, output_tex;

  // Set up OpenGL parameters as necessary
  functions_->glViewport(0, 0, params.effective_width(), params.effective_height());

  // Bind all textures
  for (int i=0; i<textures_to_bind.size(); i++) {
    functions_->glActiveTexture(GL_TEXTURE0 + i);
    functions_->glBindTexture(GL_TEXTURE_2D, textures_to_bind.at(i));
    OpenGLRenderFunctions::PrepareToDraw(functions_);
  }

  for (int iteration=0; iteration<real_iteration_count; iteration++) {
    // Set iteration number
    shader->bind();
    shader->setUniformValue("ove_iteration", iteration);
    shader->release();

    // Replace iterative input
    if (iteration == 0) {
      output_tex = dst_refs[0];
    } else {
      input_tex = dst_refs[(iteration+1)%2];
      output_tex = dst_refs[iteration%2];

      functions_->glActiveTexture(GL_TEXTURE0 + iterative_input);
      functions_->glBindTexture(GL_TEXTURE_2D, input_tex->texture()->texture());
      OpenGLRenderFunctions::PrepareToDraw(functions_);
    }

    buffer_.Attach(output_tex, true);
    buffer_.Bind();

    // Blit this texture through this shader
    OpenGLRenderFunctions::Blit(shader);

    buffer_.Release();
    buffer_.Detach();
  }

  // Release any textures we bound before
  for (int i=textures_to_bind.size()-1; i>=0; i--) {
    functions_->glActiveTexture(GL_TEXTURE0 + i);
    functions_->glBindTexture(GL_TEXTURE_2D, 0);
  }

  return output_tex;
}

/*VideoParams OpenGLRenderer::GetParamsFromTexture(QVariant texture)
{
  GLuint t = texture.value<GLuint>();

  return texture_params_.value(t);
}*/

GLint OpenGLRenderer::GetInternalFormat(PixelFormat::Format format)
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

GLenum OpenGLRenderer::GetPixelFormat(PixelFormat::Format format)
{
  if (PixelFormat::FormatHasAlphaChannel(format)) {
    return GL_RGBA;
  } else {
    return GL_RGB;
  }
}

GLenum OpenGLRenderer::GetPixelType(PixelFormat::Format format)
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

OLIVE_NAMESPACE_EXIT
