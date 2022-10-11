/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

// Fixes weird define issue when including <avfilter.h>
#include <inttypes.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <QTimer>
#include <QVector>
#include <QWaitCondition>

#include "codec/decoder.h"
#include "common/ffmpegutils.h"

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
  DECODER_DEFAULT_DESTRUCTOR(FFmpegDecoder)

  virtual QString id() const override;

  virtual bool SupportsVideo() override{return true;}
  virtual bool SupportsAudio() override{return true;}

  virtual FootageDescription Probe(const QString &filename, CancelAtom *cancelled) const override;

protected:
  virtual bool OpenInternal() override;
  virtual TexturePtr RetrieveVideoInternal(const RetrieveVideoParams& p) override;
  virtual bool ConformAudioInternal(const QVector<QString>& filenames, const AudioParams &params, CancelAtom *cancelled) override;
  virtual void CloseInternal() override;

  virtual rational GetAudioStartOffset() const override;

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

    bool IsOpen() const
    {
      return fmt_ctx_;
    }

    void Close();

    /**
     * @brief Uses the FFmpeg API to retrieve a packet (stored in pkt_) and decode it (stored in frame_)
     *
     * @return
     *
     * An FFmpeg error code, or >= 0 on success
     */
    int GetFrame(AVPacket* pkt, AVFrame* frame);

    const char *GetSubtitleHeader() const;

    int GetSubtitle(AVPacket* pkt, AVSubtitle* sub);

    int GetPacket(AVPacket *pkt);

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

  bool InitScaler(AVFrame *input, const RetrieveVideoParams &params);
  void FreeScaler();

  static VideoParams::Format GetNativePixelFormat(AVPixelFormat pix_fmt);
  static int GetNativeChannelCount(AVPixelFormat pix_fmt);

  static uint64_t ValidateChannelLayout(AVStream *stream);

  static const char* GetInterlacingModeInFFmpeg(VideoParams::Interlacing interlacing);

  static bool IsPixelFormatGLSLCompatible(AVPixelFormat f);

  AVFramePtr GetFrameFromCache(const int64_t &t) const;

  void ClearFrameCache();

  AVFramePtr RetrieveFrame(const rational &time, VideoParams::Interlacing interlacing, CancelAtom *cancelled);

  void RemoveFirstFrame();

  bool ApplyScaler(AVFrame *in);

  static int MaximumQueueSize();

  RetrieveVideoParams filter_params_;
  AVFilterGraph* filter_graph_;
  AVFilterContext* buffersrc_ctx_;
  AVFilterContext* buffersink_ctx_;
  AVPixelFormat input_fmt_;
  VideoParams::Format native_internal_pix_fmt_;
  VideoParams::Format native_output_pix_fmt_;
  int native_channel_count_;
  rational frame_rate_tb_;

  AVFrame *working_frame_;
  AVPacket *working_packet_;

  int64_t second_ts_;

  std::list<AVFramePtr> cached_frames_;

  bool cache_at_zero_;
  bool cache_at_eof_;

  Instance instance_;

};

}

#endif // FFMPEGDECODER_H
