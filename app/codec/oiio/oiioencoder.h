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

#ifndef OIIOENCODER_H
#define OIIOENCODER_H

#include "codec/encoder.h"

namespace olive {

class OIIOEncoder : public Encoder
{
  Q_OBJECT
public:
  OIIOEncoder(const EncodingParams &params);

public slots:
  virtual bool Open() override;

  virtual bool WriteFrame(olive::FramePtr frame, olive::rational time) override;
  virtual bool WriteAudio(SampleBufferPtr audio) override;
  virtual bool WriteSubtitle(const SubtitleBlock *sub_block) override;

  virtual void Close() override;

};

}

#endif // OIIOENCODER_H
