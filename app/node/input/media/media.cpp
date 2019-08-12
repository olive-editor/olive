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
#include "render/colorservice.h"
#include "render/pixelservice.h"
// End test code

MediaInput::MediaInput() :
  decoder_(nullptr)
{
  footage_input_ = new NodeInput();
  footage_input_->add_data_input(NodeInput::kFootage);
  AddParameter(footage_input_);

  texture_output_ = new NodeOutput();
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
  // FIXME: Use OCIO for color management

  // Set default texture to no texture
  texture_output_->set_value(0);

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

  FramePtr frame = decoder_->Retrieve(time);

  if (frame == nullptr) {
    return;
  }

  // Use OCIO
  frame = PixelService::ConvertPixelFormat(frame, olive::PIX_FMT_RGBA32F);
  ColorService::ConvertFrame(frame);

  // FIXME: Test code
  if (tex_buf_.IsCreated()) {
    tex_buf_.Upload(frame->data());
  } else {
    tex_buf_.Create(QOpenGLContext::currentContext(),
                    static_cast<olive::PixelFormat>(frame->format()),
                    frame->width(),
                    frame->height(),
                    frame->data());
  }

  texture_output_->set_value(tex_buf_.texture());
  // End test code
}
