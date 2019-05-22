#include "ffmpegdecoder.h"

FFmpegDecoder::FFmpegDecoder()
{

}

bool FFmpegDecoder::Open()
{
  if (open_) {
    return true;
  }


}

void FFmpegDecoder::Request(const rational &timecode)
{

}

void FFmpegDecoder::Retrieve(uint8_t **buffer, int *linesize)
{

}

void FFmpegDecoder::Close()
{
  if (!open_) {
    return;
  }
}
