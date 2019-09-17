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

#include "media.h"

#include <QDebug>
#include <QOpenGLPixelTransferOptions>

#include "decoder/ffmpeg/ffmpegdecoder.h"
#include "node/processor/renderer/renderer.h"
#include "project/item/footage/footage.h"
#include "render/gl/shadergenerators.h"
#include "render/gl/functions.h"
#include "render/pixelservice.h"

MediaInput::MediaInput() :
  decoder_(nullptr),
  color_service_(nullptr),
  pipeline_(nullptr),
  ocio_texture_(0),
  frame_(nullptr)
{
  footage_input_ = new NodeInput("footage_in");
  footage_input_->add_data_input(NodeInput::kFootage);
  AddParameter(footage_input_);

  matrix_input_ = new NodeInput("matrix_in");
  matrix_input_->add_data_input(NodeInput::kMatrix);
  AddParameter(matrix_input_);

  texture_output_ = new NodeOutput("tex_out");
  texture_output_->set_data_type(NodeOutput::kTexture);
  texture_output_->SetValueCachingEnabled(false);
  AddParameter(texture_output_);
}

QString MediaInput::Name()
{
  return tr("Media");
}

QString MediaInput::id()
{
  return "org.olivevideoeditor.Olive.mediainput";
}

QString MediaInput::Category()
{
  return tr("Input");
}

QString MediaInput::Description()
{
  return tr("Import a footage stream.");
}

void MediaInput::Release()
{
  internal_tex_.Destroy();

  frame_ = nullptr;
  decoder_ = nullptr;
  color_service_ = nullptr;
  pipeline_ = nullptr;

  if (ocio_texture_ != 0) {
    ocio_ctx_->functions()->glDeleteTextures(1, &ocio_texture_);
  }
}

NodeInput *MediaInput::matrix_input()
{
  return matrix_input_;
}

NodeOutput *MediaInput::texture_output()
{
  return texture_output_;
}

void MediaInput::SetFootage(Footage *f)
{
  footage_input_->set_value(PtrToValue(f));
}

void MediaInput::Hash(QCryptographicHash *hash, NodeOutput *from, const rational &time)
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

QVariant MediaInput::Value(NodeOutput *output, const rational &time)
{
  // FIXME: Hardcoded value
  bool alpha_is_associated = false;

  if (output == texture_output_) {
    // Find the current Renderer instance
    RenderInstance* renderer = RendererProcessor::CurrentInstance();

    // If nothing is available, don't return a texture
    if (renderer == nullptr) {
      return 0;
    }

    // Make sure decoder is set up
    if (!SetupDecoder()) {
      return 0;
    }

    // Check if we need to get a frame or not
    if (frame_ == nullptr || frame_->native_timestamp() != decoder_->GetTimestampFromTime(time)) {
      // Get frame from Decoder
      frame_ = decoder_->Retrieve(time);

      if (frame_ == nullptr) {
        qDebug() << "Received a null frame while time was" << time.toDouble();
        return 0;
      }

      if (color_service_ == nullptr) {
        // FIXME: Hardcoded values for testing
        color_service_ = std::make_shared<ColorService>("srgb", OCIO::ROLE_SCENE_LINEAR);
      }

      // OpenColorIO v1's color transforms can be done on GPU, which improves performance but reduces accuracy. When
      // online, we prefer accuracy over performance so we use the CPU path instead:
      // NOTE: OCIO v2 boasts 1:1 results with the CPU and GPU path so this won't be necessary forever
      if (renderer->mode() == olive::RenderMode::kOnline) {
        // Convert to 32F, which is required for OpenColorIO's color transformation
        frame_ = PixelService::ConvertPixelFormat(frame_, olive::PIX_FMT_RGBA32F);

        if (alpha_is_associated) {
          // Unassociate alpha here if associated
          ColorService::DisassociateAlpha(frame_);
        }

        // Transform color to reference space
        color_service_->ConvertFrame(frame_);

        if (alpha_is_associated) {
          // If alpha was associated, reassociate here
          ColorService::ReassociateAlpha(frame_);
        } else {
          // If alpha was not associated, associate here
          ColorService::AssociateAlpha(frame_);
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
                    renderer->width(),
                    renderer->height(),
                    renderer->format(),
                    RenderTexture::kDoubleBuffer);

    // Using the transformation matrix, blit our internal texture (in frame format) to our output texture (in
    // reference format)

    if (renderer->mode() == olive::RenderMode::kOffline) {
      // For offline rendering, OCIO's GPU path is acceptable:
      // NOTE: OCIO v2 boasts 1:1 results with the CPU and GPU path so this won't be necessary forever

      // Use an OCIO pipeline shader (which wraps in a default pipeline and will also handle alpha association)
      if (pipeline_ == nullptr) {
        pipeline_ = olive::ShaderGenerator::OCIOPipeline(renderer->context(),
                                                         ocio_texture_, // FIXME: A raw GLuint texture, should wrap this up
                                                         color_service_->GetProcessor(),
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
    transform.scale(2.0f / static_cast<float>(renderer->width() * renderer->divider()),
                    2.0f / static_cast<float>(renderer->height() * renderer->divider()));

    // Multiply by input transformation
    transform *= matrix_input_->get_value(time).value<QMatrix4x4>();

    // Scale texture to the media size
    transform.scale(static_cast<float>(frame_->width()), static_cast<float>(frame_->height()));
    transform.scale(0.5f, 0.5f);

    //float media_size = static_cast<float>(frame_->height()) / static_cast<float>(renderer->height() * renderer->divider());
    //transform.scale(media_size, media_size);

    // Use pipeline to blit using transformation matrix from input
    if (renderer->mode() == olive::RenderMode::kOffline) {
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

bool MediaInput::SetupDecoder()
{
  if (decoder_ != nullptr) {
    return true;
  }

  // Get currently selected Footage
  Footage* footage = ValueToPtr<Footage>(footage_input_->get_value(0));

  // If no footage is selected, return nothing
  if (footage == nullptr) {
    return false;
  }

  // Otherwise try to get frame of footage from decoder

  // Determine which decoder to use
  if (decoder_ == nullptr
      && (decoder_ = Decoder::CreateFromID(footage->decoder())) == nullptr) {
    return false;
  }

  if (decoder_->stream() == nullptr) {
    // FIXME: Hardcoded stream 0
    decoder_->set_stream(footage->stream(0));
  }

  return true;
}
