#ifndef ENCODER_H
#define ENCODER_H

#include <memory>
#include <QString>

#include "common/constructors.h"
#include "render/audioparams.h"
#include "render/videoparams.h"

class Encoder;
using EncoderPtr = std::shared_ptr<Encoder>;

class EncodingParams {
public:
  EncodingParams();

  void SetFilename(const QString& filename);
  void EnableVideo(const VideoParams& video_params, const QString& vcodec);
  void EnableAudio(const AudioParams& audio_params, const QString& acodec);

private:
  QString filename_;

  QString video_codec_;
  bool video_enabled_;
  VideoParams video_params_;

  QString audio_codec_;
  bool audio_enabled_;
  AudioParams audio_params_;
};

class Encoder
{
public:
  Encoder(const EncodingParams& params);

  DISABLE_COPY_MOVE(Encoder)

  virtual bool Open() = 0;

  virtual void Close() = 0;

private:
  EncodingParams params_;
};

#endif // ENCODER_H
