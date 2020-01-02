#ifndef AUDIOOUTPUTDEVICEPROXY_H
#define AUDIOOUTPUTDEVICEPROXY_H

#include <QIODevice>

class AudioOutputDeviceProxy : public QIODevice
{
  Q_OBJECT
public:
  AudioOutputDeviceProxy();

  void SetDevice(QIODevice* device);

  void SetSendAverages(bool send);

signals:
  void ProcessedAverages(QVector<double> averages);

protected:
  virtual qint64 readData(char *data, qint64 maxlen) override;

  virtual qint64 writeData(const char *data, qint64 maxSize) override;

private:
  QIODevice* device_;

  bool send_averages_;

};

#endif // AUDIOOUTPUTDEVICEPROXY_H
