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

#ifndef OIIODECODER_H
#define OIIODECODER_H

#include <OpenImageIO/imageio.h>

#include "decoder/decoder.h"
#include "render/pixelservice.h"

class OIIODecoder : public Decoder
{
public:
  OIIODecoder();

  virtual QString id() override;

  virtual bool Probe(Footage *f) override;

  virtual bool Open() override;

  virtual FramePtr Retrieve(const rational &timecode, const rational &length = 0) override;

  virtual void Close() override;

  virtual int64_t GetTimestampFromTime(const rational &time) override;

private:
  std::unique_ptr<OIIO::ImageInput> image_;

  int width_;

  int height_;

  olive::PixelFormat pix_fmt_;

  PixelFormatInfo pix_fmt_info_;

  bool is_rgba_;

  FramePtr frame_;

};

#endif // OIIODECODER_H
