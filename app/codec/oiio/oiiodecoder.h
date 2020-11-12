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
#include <OpenImageIO/imagebuf.h>

#include "codec/decoder.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

class OIIODecoder : public Decoder
{
  Q_OBJECT
public:
  OIIODecoder();

  virtual ~OIIODecoder() override;

  virtual QString id() override;

  virtual bool SupportsVideo() override{return true;}

  virtual FootagePtr Probe(const QString& filename, const QAtomicInt* cancelled) const override;

protected:
  virtual bool OpenInternal() override;
  virtual FramePtr RetrieveVideoInternal(const rational &timecode, const int& divider) override;
  virtual void CloseInternal() override;

private:
#if OIIO_VERSION < 10903
  OIIO::ImageInput* image_;
#else
  std::unique_ptr<OIIO::ImageInput> image_;
#endif

  static bool FileTypeIsSupported(const QString& fn);

  bool OpenImageHandler(const QString& fn);

  void CloseImageHandle();

  int64_t last_sequence_index_;

  PixelFormat::Format pix_fmt_;

  //bool is_rgba_;

  OIIO::ImageBuf* buffer_;

  static QStringList supported_formats_;

};

OLIVE_NAMESPACE_EXIT

#endif // OIIODECODER_H
