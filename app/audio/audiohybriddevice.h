#ifndef AUDIOHYBRIDDEVICE_H
#define AUDIOHYBRIDDEVICE_H

#include <QIODevice>

typedef float sample;

class AudioHybridDevice : public QIODevice
{
  Q_OBJECT
public:
  AudioHybridDevice(QObject* parent = nullptr);

  void Push(const QByteArray &samples);

  void Stop();

  void ConnectDevice(QIODevice* device);

  bool IsIdle();

signals:
  void HasSamples();

protected:
  virtual qint64 readData(char *data, qint64 maxSize) override;
  virtual qint64 writeData(const char *data, qint64 maxSize) override;

private:
  QIODevice* device_;

  QByteArray pushed_samples_;
  qint64 sample_index_;

};

#endif // AUDIOHYBRIDDEVICE_H
