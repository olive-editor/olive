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

#include <QAtomicInt>
#include <QTimer>
#include <QVector>
#include <QWaitCondition>

#include "audio/sampleformat.h"
#include "avframeptr.h"
#include "codec/decoder.h"
#include "codec/waveoutput.h"
#include "ffmpegframepool.h"
#include "project/item/footage/videostream.h"

OLIVE_NAMESPACE_ENTER

class FFmpegDecoderInstance : public QObject {
  Q_OBJECT
public:
  FFmpegDecoderInstance(const char* filename, int stream_index);
  virtual ~FFmpegDecoderInstance();

  DISABLE_COPY_MOVE(FFmpegDecoderInstance)

  bool IsValid() const;

  void SetFramePool(FFmpegFramePool* frame_pool);

  int64_t RangeStart() const;
  int64_t RangeEnd() const;
  bool CacheContainsTime(const int64_t& t) const;
  bool CacheWillContainTime(const int64_t& t) const;
  bool CacheCouldContainTime(const int64_t& t) const;
  bool CacheIsEmpty() const;
  FFmpegFramePool::ElementPtr GetFrameFromCache(const int64_t& t) const;

  void RemoveFramesBefore(const qint64& t);
  void TruncateCacheRangeTo(const qint64& t);

  AVFormatContext* fmt_ctx() const
  {
    return fmt_ctx_;
  }

  AVStream* stream() const
  {
    return avstream_;
  }

  void ClearFrameCache();

  FFmpegFramePool::ElementPtr RetrieveFrame(const int64_t &target_ts, bool cache_is_locked);

  /**
   * @brief Uses the FFmpeg API to retrieve a packet (stored in pkt_) and decode it (stored in frame_)
   *
   * @return
   *
   * An FFmpeg error code, or >= 0 on success
   */
  int GetFrame(AVPacket* pkt, AVFrame* frame);

  QMutex* cache_lock();
  QWaitCondition* cache_wait_cond();

  bool IsWorking() const;
  void SetWorking(bool working);

private:
  void ClearResources();

  void Seek(int64_t timestamp);

  AVFormatContext* fmt_ctx_;
  AVCodecContext* codec_ctx_;
  AVStream* avstream_;
  AVDictionary* opts_;

  int64_t second_ts_;

  QWaitCondition cache_wait_cond_;
  QMutex cache_lock_;
  QList<FFmpegFramePool::ElementPtr> cached_frames_;
  FFmpegFramePool* frame_pool_;

  int64_t cache_target_time_;

  bool is_working_;

  bool cache_at_zero_;
  bool cache_at_eof_;

  QTimer* clear_timer_;
  static const int kMaxFrameLife;

private slots:
  void ClearTimerEvent();

};

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

  virtual bool Probe(Footage *f, const QAtomicInt *cancelled) override;

  virtual bool Open() override;
  virtual FramePtr RetrieveVideo(const rational &timecode, const int& divider) override;
  virtual SampleBufferPtr RetrieveAudio(const rational &timecode, const rational &length, const AudioParams& params) override;
  virtual void Close() override;

  virtual QString id() override;

  virtual bool SupportsVideo() override;
  virtual bool SupportsAudio() override;

  virtual bool ConformAudio(const QAtomicInt* cancelled, const AudioParams& p) override;

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

  void ClearResources();

  void InitScaler(int divider);
  void FreeScaler();

  static int GetScaledDimension(int dim, int divider);

  static PixelFormat::Format GetNativePixelFormat(AVPixelFormat pix_fmt);

  static uint64_t ValidateChannelLayout(AVStream *stream);

  FramePtr BuffersToNativeFrame(int divider, int width, int height, int64_t ts, uint8_t **input_data, int* input_linesize);

  SwsContext* scale_ctx_;
  int scale_divider_;
  AVPixelFormat src_pix_fmt_;
  AVPixelFormat ideal_pix_fmt_;
  PixelFormat::Format native_pix_fmt_;

  rational time_base_;
  int64_t start_time_;

  static QHash< Stream*, QList<FFmpegDecoderInstance*> > instance_map_;
  static QHash< Stream*, FFmpegFramePool* > frame_pool_map_;
  static QMutex instance_map_lock_;

};

OLIVE_NAMESPACE_EXIT

#endif // FFMPEGDECODER_H
