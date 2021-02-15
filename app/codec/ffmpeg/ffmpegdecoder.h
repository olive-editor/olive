/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include <QAtomicInt>
#include <QTimer>
#include <QVector>
#include <QWaitCondition>

#include "codec/decoder.h"
#include "codec/waveoutput.h"
#include "ffmpegframepool.h"

namespace olive {

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

  virtual QString id() override;

  virtual bool SupportsVideo() override{return true;}
  virtual bool SupportsAudio() override{return true;}

  virtual Streams Probe(const QString &filename, const QAtomicInt *cancelled) const override;

protected:
  virtual bool OpenInternal() override;
  virtual FramePtr RetrieveVideoInternal(const rational &timecode, const int& divider) override;
  virtual bool ConformAudioInternal(const QString& filename, const AudioParams &params, const QAtomicInt* cancelled) override;
  virtual void CloseInternal() override;

private:
  class Instance
  {
  public:
    Instance();

    ~Instance()
    {
      Close();
    }

    bool Open(const char* filename, int stream_index);

    void Close();

    /**
     * @brief Uses the FFmpeg API to retrieve a packet (stored in pkt_) and decode it (stored in frame_)
     *
     * @return
     *
     * An FFmpeg error code, or >= 0 on success
     */
    int GetFrame(AVPacket* pkt, AVFrame* frame);

    void Seek(int64_t timestamp);

    AVFormatContext* fmt_ctx() const
    {
      return fmt_ctx_;
    }

    AVStream* avstream() const
    {
      return avstream_;
    }

  private:
    AVFormatContext* fmt_ctx_;
    AVCodecContext* codec_ctx_;
    AVStream* avstream_;
    AVDictionary* opts_;

  };

  /**
   * @brief Handle an FFmpeg error code
   *
   * Uses the FFmpeg API to retrieve a descriptive string for this error code and sends it to Error(). As such, this
   * function also automatically closes the Decoder.
   *
   * @param error_code
   */
  static QString FFmpegError(int error_code);

  void InitScaler(int divider);
  void FreeScaler();

  //FramePtr RetrieveStillImage(const rational& timecode, const int& divider);

  static VideoParams::Format GetNativePixelFormat(AVPixelFormat pix_fmt);
  static int GetNativeChannelCount(AVPixelFormat pix_fmt);

  static uint64_t ValidateChannelLayout(AVStream *stream);

  void FFmpegBufferToNativeBuffer(uint8_t** input_data, int* input_linesize, uint8_t **output_buffer, int *output_linesize);

  FFmpegFramePool::ElementPtr GetFrameFromCache(const int64_t& t) const;

  void ClearFrameCache();

  FFmpegFramePool::ElementPtr RetrieveFrame(const rational &time, int divider);

  void RemoveFirstFrame();

  SwsContext* scale_ctx_;
  int scale_divider_;
  AVPixelFormat ideal_pix_fmt_;
  VideoParams::Format native_pix_fmt_;
  int native_channel_count_;

  FFmpegFramePool pool_;

  int64_t second_ts_;

  QList<FFmpegFramePool::ElementPtr> cached_frames_;

  bool is_working_;
  QMutex is_working_mutex_;

  bool cache_at_zero_;
  bool cache_at_eof_;

  Instance instance_;

};

}

#endif // FFMPEGDECODER_H
