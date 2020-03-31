#ifndef SAMPLEBUFFER_H
#define SAMPLEBUFFER_H

#include <memory>

#include "common/constructors.h"
#include "render/audioparams.h"

class SampleBuffer;
using SampleBufferPtr = std::shared_ptr<SampleBuffer>;

/**
 * @brief A buffer of audio samples
 *
 * Audio samples in this structure are always stored in PLANAR (separated by channel). This is done to simplify audio
 * rendering code. This replaces the old system of using QByteArrays (containing packed audio) and while SampleBuffer
 * replaces many of those in the rendering/processing side of things, QByteArrays are currently still in use for
 * playback, including reading to and from the cache.
 */
class SampleBuffer
{
public:
  SampleBuffer();

  virtual ~SampleBuffer();

  static SampleBufferPtr Create();
  static SampleBufferPtr CreateAllocated(const AudioRenderingParams& audio_params, int samples_per_channel);
  static SampleBufferPtr CreateFromPackedData(const AudioRenderingParams& audio_params, const QByteArray& bytes);

  DISABLE_COPY_MOVE(SampleBuffer)

  const AudioRenderingParams& audio_params() const;
  void set_audio_params(const AudioRenderingParams& params);

  const int &sample_count_per_channel() const;
  void set_sample_count_per_channel(const int &sample_count_per_channel);

  float** data();
  float* channel_data(int channel);
  float* sample_data(int index);

  bool is_allocated() const;
  void allocate();
  void destroy();

  void reverse();
  void speed(double speed);

  QByteArray toPackedData() const;

private:
  static void allocate_sample_buffer(float*** data, int nb_channels, int nb_samples);

  static void destroy_sample_buffer(float*** data, int nb_channels);

  AudioRenderingParams audio_params_;

  int sample_count_per_channel_;

  float** data_;

};

Q_DECLARE_METATYPE(SampleBufferPtr)

#endif // SAMPLEBUFFER_H
