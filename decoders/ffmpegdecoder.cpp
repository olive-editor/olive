#include "ffmpegdecoder.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <QStatusBar>
#include <QString>
#include <QtMath>
#include <QDebug>

#include "ui/mainwindow.h"

FFmpegDecoder::FFmpegDecoder() :
  fmt_ctx_(nullptr),
  codec_ctx_(nullptr),
  opts_(nullptr)
{
}

bool FFmpegDecoder::Open()
{
  if (open_) {
    return true;
  }

  int error_code;

  QByteArray ba = stream()->footage->url.toUtf8();
  const char* filename = ba.constData();

  // Open file in a format context
  error_code = avformat_open_input(&fmt_ctx_, filename, nullptr, nullptr);

  // Handle format context error
  if (error_code != 0) {
    FFmpegErr(error_code);
    return false;
  }

  // Get stream information from format
  error_code = avformat_find_stream_info(fmt_ctx_, nullptr);

  // Handle get stream information error
  if (error_code < 0) {
    FFmpegErr(error_code);
    return false;
  }

  // Dump format information
  av_dump_format(fmt_ctx_, stream()->file_index, filename, 0);

  // Get reference to correct AVStream
  avstream_ = fmt_ctx_->streams[stream()->file_index];

  // Find decoder
  AVCodec* codec = avcodec_find_decoder(avstream_->codecpar->codec_id);

  // Handle failure to find decoder
  if (codec == nullptr) {
    Error(tr("Failed to find appropriate decoder for this codec (%1 :: %2)")
          .arg(stream()->footage->url, avstream_->codecpar->codec_id));
    return false;
  }

  // Allocate context for the decoder
  codec_ctx_ = avcodec_alloc_context3(codec);
  if (codec_ctx_ == nullptr) {
    Error(tr("Failed to allocate codec context (%1 :: %2)").arg(stream()->footage->url, stream()->file_index));
    return false;
  }

  // Copy parameters from the AVStream to the AVCodecContext
  error_code = avcodec_parameters_to_context(codec_ctx_, avstream_->codecpar);

  // Handle failure to copy parameters
  if (error_code < 0) {
    FFmpegErr(error_code);
    return false;
  }

  // enable multithreading on decoding
  error_code = av_dict_set(&opts_, "threads", "auto", 0);

  // Handle failure to set multithreaded decoding
  if (error_code < 0) {
    FFmpegErr(error_code);
    return false;
  }

  // Open codec
  error_code = avcodec_open2(codec_ctx_, codec, &opts_);
  if (error_code < 0) {
    FFmpegErr(error_code);
    return false;
  }

  open_ = true;

  return true;
}

void FFmpegDecoder::Close()
{
  if (opts_ != nullptr) {
    av_dict_free(&opts_);
    opts_ = nullptr;
  }

  if (codec_ctx_ != nullptr) {
    avcodec_free_context(&codec_ctx_);
    codec_ctx_ = nullptr;
  }

  if (fmt_ctx_ != nullptr) {
    avformat_close_input(&fmt_ctx_);
    fmt_ctx_ = nullptr;
  }

  open_ = false;
}

void FFmpegDecoder::FFmpegErr(int error_code)
{
  char err[1024];
  av_strerror(error_code, err, 1024);

  Error(tr("Error decoding %1 - %2 %3").arg(stream()->footage->url,
                                            QString::number(error_code),
                                            err));
}

void FFmpegDecoder::Error(const QString &s)
{
  qWarning() << s;
  olive::MainWindow->statusBar()->showMessage(s);

  Close();
}
