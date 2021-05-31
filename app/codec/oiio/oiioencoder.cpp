/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "oiioencoder.h"

#include "common/oiioutils.h"

namespace olive {

OIIOEncoder::OIIOEncoder(const EncodingParams &params) :
  Encoder(params)
{

}

bool OIIOEncoder::Open()
{
  return true;
}

bool OIIOEncoder::WriteFrame(FramePtr frame, rational time)
{
  std::string filename = GetFilenameForFrame(time).toStdString();

  auto output = OIIO::ImageOutput::create(filename);
  if (!output) {
    return false;
  }

  OIIO::TypeDesc type = OIIOUtils::GetOIIOBaseTypeFromFormat(frame->format());
  OIIO::ImageSpec spec(frame->width(), frame->height(), frame->channel_count(), type);

  if (!output->open(filename, spec)) {
    return false;
  }

  if (!output->write_image(type, frame->data(), OIIO::AutoStride, frame->linesize_bytes())) {
    return false;
  }

  if (!output->close()) {
    return false;
  }

  return true;
}

bool OIIOEncoder::WriteAudio(SampleBufferPtr audio)
{
  // Do nothing
  return false;
}

bool OIIOEncoder::WriteSubtitle(const SubtitleBlock *sub_block)
{
  return false;
}

void OIIOEncoder::Close()
{
  // Do nothing
}

}
