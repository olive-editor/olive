#include "bufferaverage.h"

QVector<double> AudioBufferAverage::ProcessAverages(const char *data, int length)
{
  // FIXME: Assumes float and stereo
  const float* samples = reinterpret_cast<const float*>(data);
  int sample_count = static_cast<int>(length / static_cast<int>(sizeof(float)));
  int channels = 2;

  // Create array of samples to send
  QVector<double> averages(channels);
  averages.fill(0);

  // Add all samples together
  for (int i=0;i<sample_count;i++) {
    averages[i%channels] = qMax(averages[i%channels], static_cast<double>(qAbs(samples[i])));
  }

  return averages;
}
