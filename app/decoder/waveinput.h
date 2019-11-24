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

  bool is_open();

  QByteArray read(int offset, int length);
  void read(int offset, char *buffer, int length);

  AudioRenderingParams params();

  void close();

private:
  bool find_str(QFile* f, const char* str);

  AudioRenderingParams params_;

  QFile file_;

  qint64 data_position_;
};

#endif // WAVEINPUT_H
