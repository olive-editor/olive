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

#include "project/item/footage/footage.h"

// FIXME: Test code only
#include "decoder/ffmpeg/ffmpegdecoder.h"
#include "node/processor/renderer/renderer.h"
#include "render/pixelservice.h"
#include "render/gl/shadergenerators.h"
#include "render/gl/functions.h"
// End test code

MediaInput::MediaInput() :
  //ocio_shader_(nullptr),
  decoder_(nullptr)
{
  footage_input_ = new NodeInput("footage_in");
  footage_input_->add_data_input(NodeInput::kFootage);
  AddParameter(footage_input_);

  matrix_input_ = new NodeInput("matrix_in");
  matrix_input_->add_data_input(NodeInput::kMatrix);
  AddParameter(matrix_input_);

  texture_output_ = new NodeOutput("tex_out");
  texture_output_->set_data_type(NodeOutput::kTexture);
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
  texture_.Destroy();

  decoder_ = nullptr;
}

NodeOutput *MediaInput::texture_output()
{
  return texture_output_;
}

void MediaInput::SetFootage(Footage *f)
{
  // FIXME: Need some protection for Time == 0
  footage_input_->set_value(0, PtrToValue(f));
}

void MediaInput::Process(const rational &time)
{
  // Set default texture to no texture
  texture_output_->set_value(0);

  // Find the current Renderer instance
  RenderInstance* renderer = RendererProcessor::CurrentInstance();

  // If nothing is available, don't return a texture
  if (renderer == nullptr) {
    return;
  }

  // Get currently selected Footage
  Footage* footage = ValueToPtr<Footage>(footage_input_->get_value(time));

  // If no footage is selected, return nothing
  if (footage == nullptr) {
    return;
  }

  // Otherwise try to get frame of footage from decoder

  // Determine which decoder to use
  if (decoder_ == nullptr
      && (decoder_ = Decoder::CreateFromID(footage->decoder())) == nullptr) {
    return;
  }

  if (decoder_->stream() == nullptr) {
    // FIXME: Hardcoded stream 0
    decoder_->set_stream(footage->stream(0));
  }

  // Get frame from Decoder
  FramePtr frame = decoder_->Retrieve(time);

  if (frame == nullptr) {
    return;
  }

  /*renderer->buffer()->Upload(frame->data());

  texture_output_->set_value(renderer->buffer()->texture());*/

  // Convert the frame to the Renderer format
//  frame = PixelService::ConvertPixelFormat(frame, olive::PIX_FMT_RGBA16F);

  // Convert the frame to the Renderer color space
  //color_service_.ConvertFrame(frame);

  // Upload this frame to the GPU
  /*if (buffer_.IsCreated()) {
    buffer_.Upload(frame->data());
  } else {
    buffer_.Create(QOpenGLContext::currentContext(),
                    static_cast<olive::PixelFormat>(frame->format()),
                    frame->width(),
                    frame->height(),
                    frame->data());
  }*/

  // Draw according to matrix
  // BLIT

  //texture_output_->set_value(tex_buf_.texture());
  // End test code
}
