#include "openglworker.h"

#include "core.h"
#include "functions.h"
#include "node/node.h"
#include "openglcolorprocessor.h"
#include "render/colormanager.h"
#include "render/pixelservice.h"

OpenGLWorker::OpenGLWorker(QOpenGLContext *share_ctx, OpenGLShaderCache *shader_cache, VideoRenderFrameCache *frame_cache, QObject *parent) :
  VideoRenderWorker(frame_cache, parent),
  share_ctx_(share_ctx),
  ctx_(nullptr),
  functions_(nullptr),
  shader_cache_(shader_cache)
{
  surface_.create();
}

OpenGLWorker::~OpenGLWorker()
{
  surface_.destroy();
}

bool OpenGLWorker::InitInternal()
{
  if (!VideoRenderWorker::InitInternal()) {
    return false;
  }

  // Create context object
  ctx_ = new QOpenGLContext();

  // Set share context
  ctx_->setShareContext(share_ctx_);

  // Create OpenGL context (automatically destroys any existing if there is one)
  if (!ctx_->create()) {
    qWarning() << "Failed to create OpenGL context in thread" << thread();
    return false;
  }

  ctx_->moveToThread(this->thread());

  //qDebug() << "Processor initialized in thread" << thread() << "- context is in" << ctx_->thread();

  // The rest of the initialization needs to occur in the other thread, so we signal for it to start
  QMetaObject::invokeMethod(this, "FinishInit", Qt::QueuedConnection);

  return true;
}

void OpenGLWorker::FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable *table)
{
  // Set up OCIO context
  OpenGLColorProcessorPtr color_processor = std::static_pointer_cast<OpenGLColorProcessor>(color_cache()->Get(stream.get()));

  // Ensure stream is video or image type
  if (stream->type() != Stream::kVideo && stream->type() != Stream::kImage) {
    return;
  }

  ImageStreamPtr video_stream = std::static_pointer_cast<ImageStream>(stream);

  if (!color_processor) {
    QString input_colorspace = video_stream->colorspace();
    if (input_colorspace.isEmpty()) {
      // FIXME: Should use Stream->Footage to find the Project* since that's a direct chain
      input_colorspace = olive::core.GetActiveProject()->default_input_colorspace();
    }

    color_processor = OpenGLColorProcessor::CreateOpenGL(input_colorspace,
                                                         OCIO::ROLE_SCENE_LINEAR);
    color_cache()->Add(stream.get(), color_processor);
  }

  // OCIO's CPU conversion is more accurate, so for online we render on CPU but offline we render GPU
  if (video_params().mode() == olive::kOnline) {
    // If alpha is associated, disassociate for the color transform
    if (video_stream->premultiplied_alpha()) {
      ColorManager::DisassociateAlpha(frame);
    }

    // Convert frame to float for OCIO
    frame = PixelService::ConvertPixelFormat(frame, olive::PIX_FMT_RGBA32F);

    // Perform color transform
    color_processor->ConvertFrame(frame);

    // Associate alpha
    if (video_stream->premultiplied_alpha()) {
      ColorManager::ReassociateAlpha(frame);
    } else {
      ColorManager::AssociateAlpha(frame);
    }
  }

  OpenGLTexturePtr footage_tex = std::make_shared<OpenGLTexture>();
  footage_tex->Create(ctx_, frame);

  if (video_params().mode() == olive::kOffline) {
    if (!color_processor->IsEnabled()) {
      color_processor->Enable(ctx_, video_stream->premultiplied_alpha());
    }

    // Create destination texture
    OpenGLTexturePtr associated_tex = std::make_shared<OpenGLTexture>();
    associated_tex->Create(ctx_, footage_tex->width(), footage_tex->height(), footage_tex->format());

    // Set viewport for texture size
    functions_->glViewport(0, 0, footage_tex->width(), footage_tex->height());

    buffer_.Attach(associated_tex);
    buffer_.Bind();
    footage_tex->Bind();

    // Blit old texture to new texture through OCIO shader
    color_processor->ProcessOpenGL();

    footage_tex->Release();
    buffer_.Release();
    buffer_.Detach();

    footage_tex = associated_tex;
  }

  table->Push(NodeParam::kTexture, QVariant::fromValue(footage_tex));
}

void OpenGLWorker::CloseInternal()
{
  buffer_.Destroy();
  functions_ = nullptr;
  delete ctx_;
}

void OpenGLWorker::ParametersChangedEvent()
{
  if (functions_ != nullptr && video_params().is_valid()) {
    functions_->glViewport(0, 0, video_params().effective_width(), video_params().effective_height());
  }
}

void OpenGLWorker::RunNodeAccelerated(Node *node, const NodeValueDatabase *input_params, NodeValueTable *output_params)
{
  OpenGLShaderPtr shader = shader_cache_->Get(node->id());

  if (!shader) {
    return;
  }

  // Create the output texture
  OpenGLTexturePtr output = std::make_shared<OpenGLTexture>();
  output->Create(ctx_, video_params().effective_width(), video_params().effective_height(), video_params().format());

  buffer_.Attach(output);

  buffer_.Bind();

  shader->bind();

  unsigned int input_texture_count = 0;

  foreach (NodeParam* param, node->parameters()) {
    if (param->type() == NodeParam::kInput) {
      // See if the shader has takes this parameter as an input
      int variable_location = shader->uniformLocation(param->id());

      if (variable_location > -1) {
        // This variable is used in the shader, let's set it to our value

        NodeInput* input = static_cast<NodeInput*>(param);

        // Get value from database at this input
        const NodeValueTable& input_data = (*input_params)[input];

        NodeParam::DataType find_data_type = input->data_type();

        // Exception for Footage types (try to get a Texture instead)
        if (find_data_type == NodeParam::kFootage) {
          find_data_type = NodeParam::kTexture;
        }

        // Try to get a value from it
        QVariant value = input_data.Get(find_data_type);

        switch (input->data_type()) {
        case NodeInput::kInt:
          shader->setUniformValue(variable_location, value.toInt());
          break;
        case NodeInput::kFloat:
          shader->setUniformValue(variable_location, value.toFloat());
          break;
        case NodeInput::kVec2:
          shader->setUniformValue(variable_location, value.value<QVector2D>());
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
        case NodeInput::kColor:
          shader->setUniformValue(variable_location, value.value<QColor>());
          break;
        case NodeInput::kBoolean:
          shader->setUniformValue(variable_location, value.toBool());
          break;
        case NodeInput::kFootage:
        case NodeInput::kTexture:
        {
          OpenGLTexturePtr texture = value.value<OpenGLTexturePtr>();

          functions_->glActiveTexture(GL_TEXTURE0 + input_texture_count);

          if (texture) {
            functions_->glBindTexture(GL_TEXTURE_2D, texture->texture());
          } else {
            functions_->glBindTexture(GL_TEXTURE_2D, 0);
          }

          // Set value to bound texture
          shader->setUniformValue(variable_location, input_texture_count);

          input_texture_count++;
          break;
        }
        case NodeInput::kSamples:
        case NodeInput::kText:
        case NodeInput::kRational:
        case NodeInput::kFont:
        case NodeInput::kFile:
        case NodeInput::kDecimal:
        case NodeInput::kWholeNumber:
        case NodeInput::kNumber:
        case NodeInput::kString:
        case NodeInput::kBuffer:
        case NodeInput::kVector:
        case NodeInput::kNone:
        case NodeInput::kAny:
          break;
        }
      }
    }
  }

  // Ensure viewport is correct
  functions_->glViewport(0, 0, video_params().effective_width(), video_params().effective_height());

  // Blit this texture through this shader
  olive::gl::Blit(shader);

  // Release any textures we bound before
  while (input_texture_count > 0) {
    input_texture_count--;

    // Release texture here
    functions_->glActiveTexture(GL_TEXTURE0 + input_texture_count);
    functions_->glBindTexture(GL_TEXTURE_2D, 0);
  }

  shader->release();

  buffer_.Release();

  buffer_.Detach();

  functions_->glFinish();

  output_params->Push(NodeParam::kTexture, QVariant::fromValue(output));
}

void OpenGLWorker::TextureToBuffer(const QVariant &tex_in, QByteArray &buffer)
{
  OpenGLTexturePtr texture = tex_in.value<OpenGLTexturePtr>();

  PixelFormatInfo format_info = PixelService::GetPixelFormatInfo(video_params().format());

  QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
  buffer_.Attach(texture);
  f->glBindFramebuffer(GL_READ_FRAMEBUFFER, buffer_.buffer());

  f->glReadPixels(0,
                  0,
                  texture->width(),
                  texture->height(),
                  format_info.pixel_format,
                  format_info.gl_pixel_type,
                  buffer.data());

  f->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  buffer_.Detach();
}

void OpenGLWorker::FinishInit()
{
  // Make context current on that surface
  if (!ctx_->makeCurrent(&surface_)) {
    qWarning() << "Failed to makeCurrent() on offscreen surface in thread" << thread();
    return;
  }

  // Store OpenGL functions instance
  functions_ = ctx_->functions();

  // Set up OpenGL parameters as necessary
  functions_->glEnable(GL_BLEND);
  ParametersChangedEvent();

  buffer_.Create(ctx_);
}
