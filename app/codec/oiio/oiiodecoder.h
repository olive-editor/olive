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

  virtual QString id() override;

  virtual bool Probe(Footage *f, const QAtomicInt* cancelled) override;

  virtual bool Open() override;
  virtual RetrieveState GetRetrieveState(const rational &time) override;
  virtual FramePtr RetrieveVideo(const rational &timecode, const int& divider) override;
  virtual void Close() override;

  virtual bool SupportsVideo() override;

  virtual QString GetIndexFilename() override;

  static void BufferToFrame(OIIO::ImageBuf* buf, FramePtr frame);

private:
#if OIIO_VERSION < 10903
  OIIO::ImageInput* image_;
#else
  std::unique_ptr<OIIO::ImageInput> image_;
#endif

  static bool FileTypeIsSupported(const QString& fn);

  static int GetImageSequenceDigitCount(const QString& filename);

  static QString TransformImageSequenceFileName(const QString& filename, const int64_t& number);

  static int64_t GetImageSequenceIndex(const QString& filename);

  bool OpenImageHandler(const QString& fn);

  void CloseImageHandle();

  PixelFormat::Format pix_fmt_;

  bool is_rgba_;

  bool is_sequence_;

  OIIO::ImageBuf* buffer_;

  static QStringList supported_formats_;

};

OLIVE_NAMESPACE_EXIT

#endif // OIIODECODER_H
