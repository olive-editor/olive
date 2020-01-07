#ifndef AUDIOOUTPUTDEVICEPROXY_H
#define AUDIOOUTPUTDEVICEPROXY_H

#include <QIODevice>

#include "tempoprocessor.h"

class AudioOutputDeviceProxy : public QIODevice
{
  Q_OBJECT
public:
  AudioOutputDeviceProxy();

  void SetParameters(const AudioRenderingParams& params);

  void SetDevice(QIODevice* device, int playback_speed);

  void SetSendAverages(bool send);

  virtual void close() override;

signals:
  void ProcessedAverages(QVector<double> averages);

protected:
  virtual qint64 readData(char *data, qint64 maxlen) override;

  virtual qint64 writeData(const char *data, qint64 maxSize) override;

private:
  qint64 ReverseAwareRead(char* data, qint64 maxlen);

  QIODevice* device_;

  TempoProcessor tempo_processor_;

  bool send_averages_;

  AudioRenderingParams params_;

  int playback_speed_;

};

#endif // AUDIOOUTPUTDEVICEPROXY_H
