#ifndef AUDIOBUFFERAVERAGE_H
#define AUDIOBUFFERAVERAGE_H

#include <QVector>

class AudioBufferAverage
{
public:
  AudioBufferAverage() = default;

  static QVector<double> ProcessAverages(const char* data, int length);
};

#endif // AUDIOBUFFERAVERAGE_H
