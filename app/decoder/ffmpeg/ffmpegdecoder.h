#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

#include <QVector>

#include "decoder/decoder.h"

class FFmpegDecoder : public Decoder
{
public:
  FFmpegDecoder();

  virtual bool Probe(Footage *f) override;

  virtual bool Open() override;
  virtual FramePtr Retrieve(const rational &timecode, const rational &length = 0) override;
  virtual void Close() override;

protected:
  void FFmpegErr(int error_code);
  void Error(const QString& s);

  AVFormatContext* fmt_ctx_;
  AVCodecContext* codec_ctx_;
  AVStream* avstream_;
  AVPacket* pkt_;
  AVFrame* frame_;
  AVDictionary* opts_;

  QVector<int64_t> frame_index_;

};

#endif // FFMPEGDECODER_H
