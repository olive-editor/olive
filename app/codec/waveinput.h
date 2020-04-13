/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef WAVEINPUT_H
#define WAVEINPUT_H

#include <QFile>

#include "render/audioparams.h"

OLIVE_NAMESPACE_ENTER

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
  qint64 read(int offset, char *buffer, int length);

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

OLIVE_NAMESPACE_EXIT

#endif // WAVEINPUT_H
