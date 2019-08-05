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

#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <QVector>

#include "decoder/decoder.h"

/**
 * @brief A Decoder derivative that wraps FFmpeg functions as on Olive decoder
 */
class FFmpegDecoder : public Decoder
{
public:
  FFmpegDecoder();

  virtual bool Probe(Footage *f) override;

  virtual bool Open() override;
  virtual FramePtr Retrieve(const rational &timecode, const rational &length = 0) override;
  virtual void Close() override;

private:
  void FFmpegErr(int error_code);
  void Error(const QString& s);

  void Index();
  int GetFrame();

  AVPixelFormat GetCompatiblePixelFormat(const AVPixelFormat& pix_fmt);

  AVFormatContext* fmt_ctx_;
  AVCodecContext* codec_ctx_;
  AVStream* avstream_;
  AVDictionary* opts_;
  AVFrame* frame_;
  AVPacket* pkt_;

  SwsContext* scale_ctx_;
  SwrContext* resample_ctx_;
  int output_fmt_;

  QVector<int64_t> frame_index_;

};

#endif // FFMPEGDECODER_H
