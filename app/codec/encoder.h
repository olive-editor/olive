#ifndef ENCODER_H
#define ENCODER_H

#include <memory>
#include <QString>

#include "codec/frame.h"
#include "common/constructors.h"
#include "render/audioparams.h"
#include "render/videoparams.h"

class Encoder;
using EncoderPtr = std::shared_ptr<Encoder>;

class EncodingParams {
public:
  EncodingParams();

  void SetFilename(const QString& filename);
  void EnableVideo(const VideoRenderingParams& video_params, const QString& vcodec);
  void EnableAudio(const AudioRenderingParams& audio_params, const QString& acodec);

  const QString& filename() const;

  bool video_enabled() const;
  const QString& video_codec() const;
  const VideoRenderingParams& video_params() const;

  bool audio_enabled() const;
  const QString& audio_codec() const;
  const AudioRenderingParams& audio_params() const;

private:
  QString filename_;

  bool video_enabled_;
  QString video_codec_;
  VideoRenderingParams video_params_;

  bool audio_enabled_;
  QString audio_codec_;
  AudioRenderingParams audio_params_;
};

class Encoder
{
public:
  Encoder(const EncodingParams& params);

  DISABLE_COPY_MOVE(Encoder)

  virtual bool Open() = 0;
  virtual void Write(FramePtr frame) = 0;
  virtual void Close() = 0;

  /**
   * @brief Create a Encoder instance using a Encoder ID
   *
   * @return
   *
   * A Encoder instance or nullptr if a Decoder with this ID does not exist
   */
  static EncoderPtr CreateFromID(const QString& id, const EncodingParams &params);

protected:
  const EncodingParams& params() const;

  bool open_;

private:
  EncodingParams params_;
};

#endif // ENCODER_H
