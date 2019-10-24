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
#include "audio/sampleformat.h"

/**
 * @brief A Decoder derivative that wraps FFmpeg functions as on Olive decoder
 */
class FFmpegDecoder : public Decoder
{
public:
  // Constructor
  FFmpegDecoder();

  // Destructor
  virtual ~FFmpegDecoder() override;

  virtual bool Probe(Footage *f) override;

  virtual bool Open() override;
  virtual FramePtr Retrieve(const rational &timecode, const rational &length = 0) override;
  virtual void Close() override;

  virtual QString id() override;

  virtual int64_t GetTimestampFromTime(const rational& time) override;

private:
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
  int GetFrame();

  /**
   * @brief Create an index for this media
   *
   * Indexes are used to improve speed and reliability of imported media. Calling Retrieve() will automatically check
   * for an index and create one if it doesn't exist.
   *
   * Indexing is slow so it's recommended to do it in a background thread. Index() must be called while the Decoder is
   * open, and does not automatically call Open() and Close() the Decoder. The caller must call thse manually.
   *
   * FIXME: This should perhaps become a common function for the base Decoder class
   */
  void Index();

  /**
   * @brief Returns the filename for the index
   *
   * Retrieves the absolute filename of the index file for this stream. Decoder must be open for this to work correctly.
   *
   * @return
   */
  QString GetIndexFilename();

  /**
   * @brief Used internally to load a frame index into frame_index_ (video only)
   *
   * @return
   *
   * TRUE if a frame index was successfully loaded. FALSE usually means the file didn't exist and Index() should be
   * run to create it.
   */
  bool LoadFrameIndex();

  /**
   * @brief Used in Index() to save the just created frame index to a file that can be loaded later
   */
  void SaveFrameIndex();

  int64_t GetClosestTimestampInIndex(const int64_t& ts);

  void Seek(int64_t timestamp);

  /**
   * @brief Returns an AVPixelFormat that can be
   * @param pix_fmt
   * @return
   */
  AVPixelFormat GetCompatiblePixelFormat(const AVPixelFormat& pix_fmt);

  olive::SampleFormat GetNativeSampleRate(const AVSampleFormat& smp_fmt);

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
