#ifndef WAVEINPUT_H
#define WAVEINPUT_H

#include <QFile>

#include "common/constructors.h"
#include "render/audioparams.h"

class WaveInput
{
public:
  WaveInput(const QString& f);

  ~WaveInput();

  DISABLE_COPY_MOVE(WaveInput)

  bool open();

  bool is_open() const;

  QByteArray read(int length);
  QByteArray read(int offset, int length);
  void read(int offset, char *buffer, int length);

  bool seek(qint64 pos);

  bool at_end() const;

  const AudioRenderingParams& params() const;

  void close();

  const quint32& data_length() const;

  int sample_count() const;

private:
  bool find_str(QFile* f, const char* str);

  qint64 calculate_max_read() const;

  AudioRenderingParams params_;

  QFile file_;

  qint64 data_position_;

  quint32 data_size_;
};

#endif // WAVEINPUT_H
