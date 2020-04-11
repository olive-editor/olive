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
#include "codec/decoder.h"
#include "codec/waveoutput.h"
#include "ffmpegframecache.h"
#include "project/item/footage/videostream.h"

OLIVE_NAMESPACE_ENTER

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
  virtual RetrieveState GetRetrieveState(const rational &time) override;
  virtual FramePtr RetrieveVideo(const rational &timecode, const int& divider) override;
  virtual SampleBufferPtr RetrieveAudio(const rational &timecode, const rational &length, const AudioRenderingParams& params) override;
  virtual void Close() override;

  virtual QString id() override;

  virtual bool SupportsVideo() override;
  virtual bool SupportsAudio() override;

  virtual void Index(const QAtomicInt *cancelled) override;

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
  int GetFrame(AVPacket* pkt, AVFrame* frame);

  virtual QString GetIndexFilename() override;

  void UnconditionalAudioIndex(const QAtomicInt* cancelled);

  void Seek(int64_t timestamp);

  void CacheFrameToDisk(AVFrame* f);

  void ClearFrameCache();

  void ClearResources();

  void SetupScaler(const int& divider);
  void FreeScaler();

  int64_t RangeStart() const;
  int64_t RangeEnd() const;
  bool CacheContainsTime(const int64_t& t) const;
  bool CacheWillContainTime(const rational& t) const;
  bool CacheCouldContainTime(const rational& t) const;
  AVFrame* GetFrameFromCache(const int64_t& t) const;

  AVFormatContext* fmt_ctx_;
  AVCodecContext* codec_ctx_;
  AVStream* avstream_;

  AVPixelFormat ideal_pix_fmt_;
  PixelFormat::Format native_pix_fmt_;

  SwsContext* scale_ctx_;
  int scale_divider_;

  QWaitCondition cache_wait_cond_;
  QWaitCondition cache_retrieve_cond_;
  QMutex cache_lock_;
  QMutex retrieve_lock_;
  QList<AVFrame*> cached_frames_;
  rational cache_target_time_;
  bool cache_at_zero_;
  bool cache_at_eof_;

  int64_t second_ts_;

  AVDictionary* opts_;

  //QTimer clear_timer_;

  static QHash< Stream*, QList<FFmpegDecoder*> > instances_;
  static QMutex instance_lock_;

private slots:
  //void ClearTimerEvent();

};

OLIVE_NAMESPACE_EXIT

#endif // FFMPEGDECODER_H
