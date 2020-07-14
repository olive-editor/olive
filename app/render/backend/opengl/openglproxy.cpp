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

#include "openglproxy.h"

#include <QThread>

#include "common/clamp.h"
#include "core.h"
#include "node/block/transition/transition.h"
#include "node/node.h"
#include "openglcolorprocessor.h"
#include "openglrenderfunctions.h"
#include "render/colormanager.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

OpenGLProxy* OpenGLProxy::instance_ = nullptr;

OpenGLProxy::OpenGLProxy(QObject *parent) :
  QObject(parent),
  ctx_(nullptr),
  functions_(nullptr)
{
  surface_.create();
}

OpenGLProxy::~OpenGLProxy()
{
  Close();

  surface_.destroy();
}

void OpenGLProxy::CreateInstance()
{
  instance_ = new OpenGLProxy();

  QThread* proxy_thread = new QThread();
  proxy_thread->start(QThread::IdlePriority);
  instance_->moveToThread(proxy_thread);

  if (!instance_->Init()) {
    DestroyInstance();
  }
}

void OpenGLProxy::DestroyInstance()
{
  if (instance_) {
    instance_->thread()->quit();
    instance_->thread()->wait();
    instance_->thread()->deleteLater();
    instance_->deleteLater();
    instance_ = nullptr;
  }
}

bool OpenGLProxy::Init()
{
  // Create context object
  ctx_ = new QOpenGLContext();

  // Create OpenGL context (automatically destroys any existing if there is one)
  if (!ctx_->create()) {
    qWarning() << "Failed to create OpenGL context in thread" << thread();
    return false;
  }

  ctx_->moveToThread(this->thread());

  // The rest of the initialization needs to occur in the other thread, so we signal for it to start
  QMetaObject::invokeMethod(this, "FinishInit", Qt::QueuedConnection);

  return true;
}

QVariant OpenGLProxy::FrameToValue(FramePtr frame, StreamPtr stream, const VideoParams& params, const RenderMode::Mode& mode)
{
  ImageStreamPtr video_stream = std::static_pointer_cast<ImageStream>(stream);

  // Set up OCIO context
  QString colorspace_match = video_stream->get_colorspace_match_string();

  OpenGLColorProcessorPtr color_processor = std::static_pointer_cast<OpenGLColorProcessor>(color_cache_.value(colorspace_match));

  if (!color_processor) {
    color_processor = OpenGLColorProcessor::Create(video_stream->footage()->project()->color_manager(),
                                                   video_stream->colorspace(),
                                                   video_stream->footage()->project()->color_manager()->GetReferenceColorSpace());
    color_cache_.insert(colorspace_match, color_processor);
  }

  ColorManager::OCIOMethod ocio_method = ColorManager::GetOCIOMethodForMode(mode);

  // OCIO's CPU conversion is more accurate, so for online we render on CPU but offline we render GPU
  if (ocio_method == ColorManager::kOCIOAccurate) {
    bool has_alpha = PixelFormat::FormatHasAlphaChannel(frame->format());

    // Convert frame to float for OCIO
    frame = PixelFormat::ConvertPixelFormat(frame,
                                            has_alpha
                                            ? PixelFormat::PIX_FMT_RGBA32F
                                            : PixelFormat::PIX_FMT_RGB32F);

    // If alpha is associated, disassociate for the color transform
    if (has_alpha && video_stream->premultiplied_alpha()) {
      ColorManager::DisassociateAlpha(frame);
    }

    // Perform color transform
    color_processor->ConvertFrame(frame);

    // Associate alpha
    if (has_alpha) {
      if (video_stream->premultiplied_alpha()) {
        ColorManager::ReassociateAlpha(frame);
      } else {
        ColorManager::AssociateAlpha(frame);
      }
    }
  }

  OpenGLTextureCache::ReferencePtr footage_tex_ref = texture_cache_.Get(ctx_, frame);

  if (ocio_method == ColorManager::kOCIOFast) {
    if (!color_processor->IsEnabled()) {
      color_processor->Enable(ctx_, video_stream->premultiplied_alpha());
    }

    VideoParams frame_params = frame->video_params();

    // Check frame aspect ratio
    if (frame->sample_aspect_ratio() != 1 && frame->sample_aspect_ratio() != 0) {
      int new_width = frame_params.width();
      int new_height = frame_params.height();

      // Scale the frame in a way that does not reduce the resolution
      if (frame->sample_aspect_ratio() > 1) {
        // Make wider
        new_width = qRound(static_cast<double>(new_width) * frame->sample_aspect_ratio().toDouble());
      } else {
        // Make taller
        new_height = qRound(static_cast<double>(new_height) / frame->sample_aspect_ratio().toDouble());
      }

      frame_params = VideoParams(new_width,
                                 new_height,
                                 frame_params.format(),
                                 frame_params.divider());
    }

    PixelFormat::Format texture_fmt;
    if (PixelFormat::FormatHasAlphaChannel(frame_params.format())) {
      texture_fmt = PixelFormat::GetFormatWithAlphaChannel(params.format());
    } else {
      texture_fmt = PixelFormat::GetFormatWithoutAlphaChannel(params.format());
    }

    VideoParams dest_params(frame_params.width(),
                            frame_params.height(),
                            texture_fmt,
                            frame_params.divider());

    // Create destination texture
    OpenGLTextureCache::ReferencePtr associated_tex_ref = texture_cache_.Get(ctx_, dest_params);

    buffer_.Attach(associated_tex_ref->texture(), true);
    buffer_.Bind();
    footage_tex_ref->texture()->Bind();

    // Set viewport for texture size
    functions_->glViewport(0, 0, associated_tex_ref->texture()->width(), associated_tex_ref->texture()->height());

    // Blit old texture to new texture through OCIO shader
    color_processor->ProcessOpenGL();

    footage_tex_ref->texture()->Release();
    buffer_.Release();
    buffer_.Detach();

    footage_tex_ref = associated_tex_ref;
  }

  return QVariant::fromValue(footage_tex_ref);
}

QVariant OpenGLProxy::PreCachedFrameToValue(FramePtr frame)
{
  return QVariant::fromValue(texture_cache_.Get(ctx_, frame));
}

OpenGLShaderPtr OpenGLProxy::ResolveShaderFromCache(const Node *node, const QString &shader_id)
{
  // Make a composite of the node ID and the shader ID (if applicable)
  QString full_shader_id = QStringLiteral("%1:%2").arg(node->id(), shader_id);
  OpenGLShaderPtr shader = shader_cache_.value(full_shader_id);

  if (!shader) {
    // Since we have shader code, compile it now
    ShaderCode code = node->GetShaderCode(shader_id);
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

    shader = OpenGLShader::Create();
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
  }

  return shader;
}

void OpenGLProxy::Close()
{
  shader_cache_.clear();
  buffer_.Destroy();
  copy_pipeline_ = nullptr;
  functions_ = nullptr;
  delete ctx_;
  ctx_ = nullptr;
}

QVariant OpenGLProxy::RunNodeAccelerated(const Node *node,
                                         const TimeRange &range,
                                         const ShaderJob &job,
                                         const VideoParams& params)
{
  // If this node is iterative, we'll pick up which input here
  GLuint iterative_input = 0;
  QList<GLuint> textures_to_bind;
  bool input_textures_have_alpha = false;

  OpenGLShaderPtr shader = ResolveShaderFromCache(node, job.GetShaderID());

  if (!shader) {
    return QVariant();
  }

  shader->bind();

  NodeValueMap::const_iterator it;
  for (it=job.GetValues().constBegin(); it!=job.GetValues().constEnd(); it++) {
    // See if the shader has takes this parameter as an input
    int variable_location = shader->uniformLocation(it.key()->id());

    if (variable_location == -1) {
      continue;
    }

    // This variable is used in the shader, let's set it
    const QVariant& value = it.value().data();

    NodeParam::DataType data_type = (it.value().type() != NodeParam::kNone)
        ? it.value().type()
        : it.key()->data_type();

    switch (data_type) {
    case NodeInput::kInt:
      shader->setUniformValue(variable_location, value.toInt());
      break;
    case NodeInput::kFloat:
      shader->setUniformValue(variable_location, value.toFloat());
      break;
    case NodeInput::kVec2:
      if (it.key()->IsArray()) {
        QVector<NodeValue> nv = value.value< QVector<NodeValue> >();
        QVector<QVector2D> a(nv.size());

        for (int j=0;j<a.size();j++) {
          a[j] = nv.at(j).data().value<QVector2D>();
        }

        shader->setUniformValueArray(variable_location, a.constData(), a.size());

        int count_location = shader->uniformLocation(QStringLiteral("%1_count").arg(it.key()->id()));
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
      OpenGLTextureCache::ReferencePtr texture = value.value<OpenGLTextureCache::ReferencePtr>();

      if (texture) {
        if (PixelFormat::FormatHasAlphaChannel(texture->texture()->format())) {
          input_textures_have_alpha = true;
        }
      }

      // Set value to bound texture
      shader->setUniformValue(variable_location, textures_to_bind.size());

      // If this texture binding is the iterative input, set it here
      if (it.key() == job.GetIterativeInput()) {
        iterative_input = textures_to_bind.size();
      }

      GLuint tex_id = texture ? texture->texture()->texture() : 0;
      textures_to_bind.append(tex_id);

      // Set enable flag if shader wants it
      int enable_param_location = shader->uniformLocation(QStringLiteral("%1_enabled").arg(it.key()->id()));
      if (enable_param_location > -1) {
        shader->setUniformValue(enable_param_location,
                                tex_id > 0);
      }

      if (tex_id > 0) {
        // Set texture resolution if shader wants it
        int res_param_location = shader->uniformLocation(QStringLiteral("%1_resolution").arg(it.key()->id()));
        if (res_param_location > -1) {
          shader->setUniformValue(res_param_location,
                                  static_cast<GLfloat>(texture->texture()->width() * texture->texture()->divider()),
                                  static_cast<GLfloat>(texture->texture()->height() * texture->texture()->divider()));
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

  if (node->IsBlock() && static_cast<const Block*>(node)->type() == Block::kTransition) {
    const TransitionBlock* transition_node = static_cast<const TransitionBlock*>(node);

    // Provides total transition progress from 0.0 (start) - 1.0 (end)
    shader->setUniformValue("ove_tprog_all", static_cast<GLfloat>(transition_node->GetTotalProgress(range.in())));

    // Provides progress of out section from 1.0 (start) - 0.0 (end)
    shader->setUniformValue("ove_tprog_out", static_cast<GLfloat>(transition_node->GetOutProgress(range.in())));

    // Provides progress of in section from 0.0 (start) - 1.0 (end)
    shader->setUniformValue("ove_tprog_in", static_cast<GLfloat>(transition_node->GetInProgress(range.in())));
  }

  shader->release();

  // Create the output textures
  PixelFormat::Format output_format = (input_textures_have_alpha || job.GetAlphaChannelRequired())
      ? PixelFormat::GetFormatWithAlphaChannel(params.format())
      : PixelFormat::GetFormatWithoutAlphaChannel(params.format());
  VideoParams output_params(params.width(),
                            params.height(),
                            params.time_base(),
                            output_format,
                            params.divider());

  int real_iteration_count;
  if (job.GetIterationCount() > 1 && job.GetIterativeInput()) {
    real_iteration_count = job.GetIterationCount();
  } else {
    real_iteration_count = 1;
  }

  OpenGLTextureCache::ReferencePtr dst_refs[2];
  dst_refs[0] = texture_cache_.Get(ctx_, output_params);

  // If this node requires multiple iterations, get a texture for it too
  if (real_iteration_count > 1) {
    dst_refs[1] = texture_cache_.Get(ctx_, output_params);
  }

  // Some nodes use multiple iterations for optimization
  OpenGLTextureCache::ReferencePtr input_tex, output_tex;

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

    buffer_.Attach(output_tex->texture(), true);
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

  return QVariant::fromValue(output_tex);
}

void OpenGLProxy::TextureToBuffer(const QVariant& tex_in,
                                  FramePtr frame,
                                  const QMatrix4x4& matrix)
{
  OpenGLTextureCache::ReferencePtr texture = tex_in.value<OpenGLTextureCache::ReferencePtr>();

  if (!texture) {
    return;
  }

  OpenGLTextureCache::ReferencePtr download_tex;

  functions_->glViewport(0, 0, frame->width(), frame->height());

  if (frame->width() != texture->texture()->width()
      || frame->height() != texture->texture()->height()) {

    // Resize the texture if necessary
    OpenGLTextureCache::ReferencePtr resized = texture_cache_.Get(ctx_, frame->video_params());

    buffer_.Attach(resized->texture(), true);
    buffer_.Bind();

    texture->texture()->Bind();

    // Blit to this new texture
    OpenGLRenderFunctions::Blit(copy_pipeline_, false, matrix);

    texture->texture()->Release();

    buffer_.Release();
    buffer_.Detach();

    download_tex = resized;

  } else {

    download_tex = texture;

  }

  buffer_.Attach(download_tex->texture());
  buffer_.Bind();

  functions_->glPixelStorei(GL_PACK_ROW_LENGTH, frame->linesize_pixels());

  functions_->glReadPixels(0,
                           0,
                           frame->width(),
                           frame->height(),
                           OpenGLRenderFunctions::GetPixelFormat(frame->format()),
                           OpenGLRenderFunctions::GetPixelType(frame->format()),
                           frame->data());

  functions_->glPixelStorei(GL_PACK_ROW_LENGTH, 0);

  buffer_.Release();
  buffer_.Detach();
}

void OpenGLProxy::FinishInit()
{
  // Make context current on that surface
  if (!ctx_->makeCurrent(&surface_)) {
    qWarning() << "Failed to makeCurrent() on offscreen surface in thread" << thread();
    return;
  }

  // Store OpenGL functions instance
  functions_ = ctx_->functions();
  functions_->glBlendFunc(GL_ONE, GL_ZERO);

  buffer_.Create(ctx_);

  copy_pipeline_ = OpenGLShader::CreateDefault();
}

OLIVE_NAMESPACE_EXIT
