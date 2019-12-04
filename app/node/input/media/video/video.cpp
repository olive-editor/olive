#include "video.h"

#include <QDebug>
#include <QOpenGLPixelTransferOptions>

#include "core.h"
#include "decoder/ffmpeg/ffmpegdecoder.h"
#include "project/item/footage/footage.h"
#include "render/pixelservice.h"

VideoInput::VideoInput()
{
  matrix_input_ = new NodeInput("matrix_in");
  matrix_input_->set_data_type(NodeInput::kMatrix);
  AddParameter(matrix_input_);

  texture_output_ = new NodeOutput("tex_out");
  AddParameter(texture_output_);
}

Node *VideoInput::copy() const
{
  return new VideoInput();
}

QString VideoInput::Name() const
{
  return tr("Video Input");
}

QString VideoInput::id() const
{
  return "org.olivevideoeditor.Olive.videoinput";
}

QString VideoInput::Category() const
{
  return tr("Input");
}

QString VideoInput::Description() const
{
  return tr("Import a video footage stream.");
}

void VideoInput::Release()
{
  MediaInput::Release();
}

NodeInput *VideoInput::matrix_input() const
{
  return matrix_input_;
}

NodeOutput *VideoInput::texture_output() const
{
  return texture_output_;
}

QString VideoInput::Code(NodeOutput *output) const
{
  if (output == texture_output()) {
    return "#version 110\n"
           "\n"
           "varying vec2 v_texcoord;\n"
           "\n"
           "uniform sampler2D footage_in;\n"
           "uniform mat4 matrix_in;\n"
           "\n"
           "void main(void) {\n"
           "  gl_FragColor = texture2D(footage_in, vec2(vec4(v_texcoord, 0.0, 1.0) * matrix_in));\n"
           "}\n";
  }

  return Node::Code(output);
}

/*
void VideoInput::Hash(QCryptographicHash *hash, NodeOutput *from, const rational &time)
{
  Node::Hash(hash, from, time);

  // Use frame value from Decoder
  if (from == texture_output_) {
    if (!SetupDecoder()) {
      qDebug() << "Failed to setup decoder for hashing";
      return;
    }

    int64_t timestamp = decoder_->GetTimestampFromTime(time);

    QByteArray pts_bytes;
    pts_bytes.resize(sizeof(int64_t));
    memcpy(pts_bytes.data(), &timestamp, sizeof(int64_t));

    hash->addData(pts_bytes);
    // FIXME: Add OCIO data
    // FIXME: Add alpha association value
  }
}

QVariant VideoInput::Value(NodeOutput *output, const rational &in, const rational &out)
{
  Q_UNUSED(out)

  if (output == texture_output_) {
    if (footage() == nullptr
        || (footage()->type() != Stream::kVideo && footage()->type() != Stream::kImage)) {
      return 0;
    }

    // FIXME: Hardcoded value
    bool alpha_is_associated = false;

    // Find the current Renderer instance
    RenderInstance* renderer = VideoRendererProcessor::CurrentInstance();

    // If nothing is available, don't return a texture
    if (renderer == nullptr) {
      return 0;
    }

    // Make sure decoder is set up
    if (!SetupDecoder()) {
      return 0;
    }

    // Check if we need to get a frame or not
    if (frame_ == nullptr || frame_->native_timestamp() != decoder_->GetTimestampFromTime(in)) {
      // Get frame from Decoder
      frame_ = decoder_->Retrieve(in);

      if (frame_ == nullptr) {
        qDebug() << "Received a null frame while time was" << in.toDouble();
        return 0;
      }

      if (color_processor_ == nullptr) {
        QString colorspace = std::static_pointer_cast<VideoStream>(footage())->colorspace();
        if (colorspace.isEmpty()) {
          // FIXME: Should use Footage() to find the Project* it belongs to instead of this
          colorspace = olive::core.GetActiveProject()->default_input_colorspace();
        }

        color_processor_ = ColorProcessor::Create(colorspace, OCIO::ROLE_SCENE_LINEAR);
      }

      // OpenColorIO v1's color transforms can be done on GPU, which improves performance but reduces accuracy. When
      // online, we prefer accuracy over performance so we use the CPU path instead:
      // NOTE: OCIO v2 boasts 1:1 results with the CPU and GPU path so this won't be necessary forever
      if (renderer->params().mode() == olive::RenderMode::kOnline) {
        // Convert to 32F, which is required for OpenColorIO's color transformation
        frame_ = PixelService::ConvertPixelFormat(frame_, olive::PIX_FMT_RGBA32F);

        if (alpha_is_associated) {
          // Unassociate alpha here if associated
          ColorManager::DisassociateAlpha(frame_);
        }

        // Transform color to reference space
        color_processor_->ConvertFrame(frame_);

        if (alpha_is_associated) {
          // If alpha was associated, reassociate here
          ColorManager::ReassociateAlpha(frame_);
        } else {
          // If alpha was not associated, associate here
          ColorManager::AssociateAlpha(frame_);
        }
      }

      // We use an internal texture to bring the texture into GPU space before performing transformations

      // Ensure the texture is the accurate to the frame
      if (internal_tex_.width() != frame_->width()
          || internal_tex_.height() != frame_->height()
          || internal_tex_.format() != frame_->format()) {
        internal_tex_.Destroy();
      }

      // Create or upload the new data to the texture
      if (!internal_tex_.IsCreated()) {
        internal_tex_.Create(renderer->context(),
                             frame_->width(),
                             frame_->height(),
                             static_cast<olive::PixelFormat>(frame_->format()),
                             frame_->data());
      } else {
        internal_tex_.Upload(frame_->data());
      }
    }

    // Create new texture in reference space to send throughout the rest of the graph

    RenderTexturePtr output_texture = std::make_shared<RenderTexture>();

    output_texture->Create(renderer->context(),
                           renderer->params().effective_width(),
                           renderer->params().effective_height(),
                           renderer->params().format(),
                           RenderTexture::kDoubleBuffer);

    // Using the transformation matrix, blit our internal texture (in frame format) to our output texture (in
    // reference format)

    if (renderer->params().mode() == olive::RenderMode::kOffline) {
      // For offline rendering, OCIO's GPU path is acceptable:
      // NOTE: OCIO v2 boasts 1:1 results with the CPU and GPU path so this won't be necessary forever

      // Use an OCIO pipeline shader (which wraps in a default pipeline and will also handle alpha association)
      if (pipeline_ == nullptr) {
        pipeline_ = olive::ShaderGenerator::OCIOPipeline(renderer->context(),
                                                         ocio_texture_, // FIXME: A raw GLuint texture, should wrap this up
                                                         color_processor_->GetProcessor(),
                                                         alpha_is_associated);

        // Used for cleanup later
        ocio_ctx_ = renderer->context();
      }
    } else if (pipeline_ == nullptr) {
      // In online, the color transformation was performed on the CPU (see above), so we only need to blit
      pipeline_ = olive::ShaderGenerator::DefaultPipeline();
    }

    renderer->context()->functions()->glBlendFunc(GL_ONE, GL_ZERO);

    // Draw onto the output texture using the renderer's framebuffer
    renderer->buffer()->Attach(output_texture);
    renderer->buffer()->Bind();

    // Draw with the internal texture
    internal_tex_.Bind();

    QMatrix4x4 transform;

    // Scale texture to a square for incoming matrix transformation
    transform.scale(2.0f / static_cast<float>(renderer->params().width()),
                    2.0f / static_cast<float>(renderer->params().height()));

    // Multiply by input transformation
    transform *= matrix_input_->get_value(in).value<QMatrix4x4>();

    // Scale texture to the media size
    transform.scale(static_cast<float>(frame_->width()), static_cast<float>(frame_->height()));
    transform.scale(0.5f, 0.5f);

    // Use pipeline to blit using transformation matrix from input
    if (renderer->params().mode() == olive::RenderMode::kOffline) {
      olive::gl::OCIOBlit(pipeline_, ocio_texture_, false, transform);
    } else {
      olive::gl::Blit(pipeline_, false, transform);
    }

    // Release everything
    internal_tex_.Release();
    renderer->buffer()->Detach();
    renderer->buffer()->Release();

    return QVariant::fromValue(output_texture);
  }

  return 0;
}
*/
