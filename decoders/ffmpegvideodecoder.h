#ifndef FFMPEGVIDEODECODER_H
#define FFMPEGVIDEODECODER_H

#include "ffmpegdecoder.h"

class FFmpegVideoDecoder : public FFmpegDecoder
{
public:
  FFmpegVideoDecoder();

  virtual FramePtr Retrieve(const rational& timecode, const rational& length = 0);

private:
};

#endif // FFMPEGVIDEODECODER_H
