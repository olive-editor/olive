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
// End test code

MediaInput::MediaInput() :
  texture_(0)
{
  footage_input_ = new NodeInput();
  footage_input_->add_data_input(NodeInput::kFootage);
  AddParameter(footage_input_);

  texture_output_ = new NodeOutput();
  texture_output_->set_data_type(NodeOutput::kTexture);
  AddParameter(texture_output_);

  // FIXME: Test code only
  decoder_ = new FFmpegDecoder(); // FIXME: Doesn't ever get free'd
  // End test code
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

NodeOutput *MediaInput::texture_output()
{
  return texture_output_;
}

void MediaInput::Process(const rational &time)
{
  // FIXME: Use OIIO and OCIO here

  // Get currently selected Footage
  Footage* footage = ValueToPtr<Footage>(footage_input_->get_value(time));

  // If no footage is selected, return nothing
  if (footage == nullptr) {
    texture_output_->set_value(0);

    return;
  }

  // Otherwise try to get frame of footage from decoder

  // Determine which decoder to use

  // Grab frame from decoder

  if (decoder_->stream() == nullptr) {
    decoder_->set_stream(footage->stream(0));
  }

  FramePtr frame = decoder_->Retrieve(time);

  if (frame == nullptr) {
    texture_output_->set_value(0);

    return;
  }

  // FIXME: Test code
  if (texture_ == 0) {
    glGenTextures(1, &texture_);

    glBindTexture(GL_TEXTURE_2D, texture_);

    // Set texture filtering to bilinear
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Set texture wrapping to clamp
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA8,
                 frame->width(),
                 frame->height(),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 frame->data()[0]);
  }

  texture_output_->set_value(texture_);
  // End test code
}
