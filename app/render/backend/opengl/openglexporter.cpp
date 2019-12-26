#include "openglexporter.h"

#include "render/backend/opengl/openglrenderfunctions.h"
#include "render/pixelservice.h"

OpenGLExporter::OpenGLExporter(ViewerOutput* viewer, const VideoRenderingParams& video_params, const AudioRenderingParams &audio_params, const QMatrix4x4 &transform, ColorProcessorPtr color_processor, Encoder *encoder, QObject* parent) :
  Exporter(viewer, video_params, audio_params, transform, color_processor, encoder, parent)
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
  texture_->Create(ctx, video_params_.effective_width(), video_params_.effective_height(), video_params_.format());

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
  frame->set_width(video_params_.width());
  frame->set_height(video_params_.height());
  frame->set_format(video_params_.format());
  frame->allocate();

  // Blit for transform if the width/height are different
  OpenGLTexturePtr input_tex = texture.value<OpenGLTexturePtr>();
  if (input_tex) {
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glViewport(0, 0, texture_->width(), texture_->height());

    buffer_.Attach(texture_);
    buffer_.Bind();
    input_tex->Bind();

    OpenGLRenderFunctions::Blit(pipeline_, false, transform_);

    input_tex->Release();
    buffer_.Release();
    buffer_.Detach();

    // Perform OpenGL read
    buffer_.Attach(texture_);
    buffer_.Bind();

    PixelFormat::Info format_info = PixelService::GetPixelFormatInfo(video_params_.format());

    f->glReadPixels(0,
                    0,
                    texture_->width(),
                    texture_->height(),
                    format_info.pixel_format,
                    format_info.gl_pixel_type,
                    frame->data());

    buffer_.Release();
    buffer_.Detach();
  }

  return frame;
}
