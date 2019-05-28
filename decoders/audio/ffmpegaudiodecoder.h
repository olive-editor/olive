#ifndef FFMPEGAUDIODECODER_H
#define FFMPEGAUDIODECODER_H

#include "decoders/ffmpegdecoder.h"

/**
 * @brief The FFmpegAudioDecoder class
 *
 * The role of an audio decoder is to simply convert audio to
 */
class FFmpegAudioDecoder : public FFmpegDecoder
{
public:
  FFmpegAudioDecoder();
};

#endif // FFMPEGAUDIODECODER_H
