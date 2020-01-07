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

class Encoder : public QObject
{
  Q_OBJECT
public:
  Encoder(const EncodingParams& params);

  /**
   * @brief Create a Encoder instance using a Encoder ID
   *
   * @return
   *
   * A Encoder instance or nullptr if a Decoder with this ID does not exist
   */
  static Encoder *CreateFromID(const QString& id, const EncodingParams &params);

  const EncodingParams& params() const;

public slots:
  void Open();
  void WriteFrame(FramePtr frame);
  virtual void WriteAudio(const AudioRenderingParams& pcm_info, const QString& pcm_filename) = 0;
  void Close();

signals:
  void OpenSucceeded();
  void OpenFailed();

  void Closed();

  void AudioComplete();

protected:
  virtual bool OpenInternal() = 0;
  virtual void WriteInternal(FramePtr frame) = 0;
  virtual void CloseInternal() = 0;

  bool IsOpen() const;

private:
  EncodingParams params_;

  bool open_;
};

#endif // ENCODER_H
