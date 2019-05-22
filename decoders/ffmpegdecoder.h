#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

#include "decoder.h"

class FFmpegDecoder : public Decoder
{
public:
  FFmpegDecoder();

  virtual bool Open();
  virtual void Request(const rational& timecode);
  virtual void Retrieve(uint8_t** buffer, int* linesize);
  virtual void Close();

private:
  AVFormatContext* fmt_ctx_;
  AVCodecContext* codec_ctx_;
  AVStream* stream_;
  AVPacket* pkt_;
  AVFrame* frame_;
};

#endif // FFMPEGDECODER_H
