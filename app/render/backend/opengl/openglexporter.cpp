#include "openglexporter.h"

#include "render/backend/opengl/functions.h"

OpenGLExporter::OpenGLExporter(ViewerOutput* viewer, const VideoRenderingParams& params, const QMatrix4x4& transform, ColorProcessorPtr color_processor, QObject* parent) :
  Exporter(viewer, params, transform, color_processor, parent)
{
}

OpenGLExporter::~OpenGLExporter()
{
}

bool OpenGLExporter::Initialize()
{
  QOpenGLContext* ctx = QOpenGLContext::currentContext();

  // Create rendering backends
  video_backend_ = new OpenGLBackend();
  audio_backend_ = new AudioBackend();

  // Create blitting framebuffer and texture
  buffer_.Create(ctx);

  texture_ = std::make_shared<OpenGLTexture>();
  texture_->Create(ctx, params_.effective_width(), params_.effective_height(), params_.format());

  pipeline_ = OpenGLShader::CreateDefault();

  return true;
}

void OpenGLExporter::Cleanup()
{
  texture_->Destroy();
  buffer_.Destroy();
}

FramePtr OpenGLExporter::TextureToFrame(const QVariant& texture)
{
  FramePtr frame = Frame::Create();
  frame->set_width(params_.width());
  frame->set_height(params_.height());
  frame->set_format(params_.format());
  frame->allocate();

  // Blit for transform if the width/height are different
  OpenGLTexturePtr input_tex = texture.value<OpenGLTexturePtr>();
  if (input_tex) {
    buffer_.Attach(texture_);
    buffer_.Bind();
    input_tex->Bind();

    olive::gl::Blit(pipeline_, false, transform_);

    input_tex->Release();
    buffer_.Release();
    buffer_.Detach();
  }

  // Perform OpenGL read

  return frame;
}
