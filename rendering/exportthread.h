/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#ifndef EXPORTTHREAD_H
#define EXPORTTHREAD_H

#include <QThread>
#include <QOffscreenSurface>
#include <QMutex>
#include <QWaitCondition>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVStream;
struct AVCodec;
struct SwsContext;
struct SwrContext;

extern "C" {
#include <libavcodec/avcodec.h>
}

#define COMPRESSION_TYPE_CBR 0
#define COMPRESSION_TYPE_CFR 1
#define COMPRESSION_TYPE_TARGETSIZE 2
#define COMPRESSION_TYPE_TARGETBR 3

// structs that store parameters passed from the export dialogs to this thread

struct ExportParams {
  // export parameters
  QString filename;
  bool video_enabled;
  int video_codec;
  int video_width;
  int video_height;
  double video_frame_rate;
  int video_compression_type;
  double video_bitrate;
  bool audio_enabled;
  int audio_codec;
  int audio_sampling_rate;
  int audio_bitrate;
  long start_frame;
  long end_frame;
};

struct VideoCodecParams {
  int pix_fmt;
  int threads;
};

class ExportThread : public QThread {
  Q_OBJECT
public:
  ExportThread(const ExportParams& params, const VideoCodecParams& vparams, QObject* parent = nullptr);
  virtual void run() override;

  const QString& GetError();

  bool WasInterrupted();
signals:
  void ProgressChanged(int value, qint64 remaining_ms);
public slots:
  void Interrupt();

  void play_wake();
private:
  bool Encode(AVFormatContext* ofmt_ctx, AVCodecContext* codec_ctx, AVFrame* frame, AVPacket* packet, AVStream* stream);
  bool SetupVideo();
  bool SetupAudio();
  bool SetupContainer();
  void Export();
  void Cleanup();

  QOffscreenSurface surface;
  bool interrupt_;

  // params imported from dialogs
  ExportParams params_;
  VideoCodecParams vcodec_params_;

  AVFormatContext* fmt_ctx;
  AVStream* video_stream;
  AVCodec* vcodec;
  AVCodecContext* vcodec_ctx;
  AVFrame* video_frame;
  AVFrame* sws_frame;
  SwsContext* sws_ctx;
  AVStream* audio_stream;
  AVCodec* acodec;
  AVFrame* audio_frame;
  AVFrame* swr_frame;
  AVCodecContext* acodec_ctx;
  AVPacket video_pkt;
  AVPacket audio_pkt;
  SwrContext* swr_ctx;

  bool vpkt_alloc;
  bool apkt_alloc;

  int aframe_bytes;
  int ret;
  char* c_filename;

  QMutex mutex;
  QWaitCondition waitCond;

  QString export_error;

  bool waiting_for_audio_;
private slots:
  void wake();
};

#endif // EXPORTTHREAD_H
