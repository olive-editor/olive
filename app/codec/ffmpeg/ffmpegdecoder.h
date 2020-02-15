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

#include "audio/sampleformat.h"
#include "codec/decoder.h"
#include "codec/waveoutput.h"
#include "project/item/footage/videostream.h"

/**
 * @brief A Decoder derivative that wraps FFmpeg functions as on Olive decoder
 */
class FFmpegDecoder : public Decoder
{
  Q_OBJECT
public:
  // Constructor
  FFmpegDecoder();

  // Destructor
  virtual ~FFmpegDecoder() override;

  virtual bool Probe(Footage *f) override;

  virtual bool Open() override;
  virtual FramePtr RetrieveVideo(const rational &timecode) override;
  virtual FramePtr RetrieveAudio(const rational &timecode, const rational &length, const AudioRenderingParams& params) override;
  virtual void Close() override;

  virtual QString id() override;

  virtual int64_t GetTimestampFromTime(const rational& time) override;

  virtual void Conform(const AudioRenderingParams& params) override;

  virtual bool SupportsVideo() override;
  virtual bool SupportsAudio() override;

  void SetMultithreading(bool e);

  virtual void Index() override;

private:
  void ConformInternal(SwrContext *resampler, WaveOutput *output, const char *in_data, int in_sample_count);

  /**
   * @brief Handle an error
   *
   * Immediately closes the Decoder (freeing memory resources) and sends the string provided to the warning stream.
   * As this function closes the Decoder, no further Decoder functions should be performed after this is called
   * (unless the Decoder is opened again first).
   */
  void Error(const QString& s);

  /**
   * @brief Handle an FFmpeg error code
   *
   * Uses the FFmpeg API to retrieve a descriptive string for this error code and sends it to Error(). As such, this
   * function also automatically closes the Decoder.
   *
   * @param error_code
   */
  void FFmpegError(int error_code);

  /**
   * @brief Uses the FFmpeg API to retrieve a packet (stored in pkt_) and decode it (stored in frame_)
   *
   * @return
   *
   * An FFmpeg error code, or >= 0 on success
   */
  int GetFrame(AVPacket* pkt, AVFrame* frame);

  /**
   * @brief Returns the filename for the index
   *
   * Retrieves the absolute filename of the index file for this stream. Decoder must be open for this to work correctly.
   */
  QString GetIndexFilename();

  /**
   * @brief Get the destination filename of an audio stream conformed to a set of parameters
   */
  QString GetConformedFilename(const AudioRenderingParams &params);

  void UnconditionalAudioIndex(AVPacket* pkt, AVFrame* frame);
  void UnconditionalVideoIndex(AVPacket* pkt, AVFrame* frame);

  int64_t GetClosestTimestampInIndex(const int64_t& ts);

  void ValidateVideoIndex();

  void Seek(int64_t timestamp);

  void CacheFrameToDisk(AVFrame* f);

  AVFormatContext* fmt_ctx_;
  AVCodecContext* codec_ctx_;
  AVStream* avstream_;

  AVPixelFormat ideal_pix_fmt_;
  PixelFormat::Format native_pix_fmt_;

  SwsContext* scale_ctx_;

  AVPacket* pkt_;

  AVFrame* frame_;

  AVDictionary* opts_;

  bool multithreading_;

};

#endif // FFMPEGDECODER_H
