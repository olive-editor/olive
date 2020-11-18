/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "waveinput.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <QDataStream>
#include <QtMath>

namespace olive {

WaveInput::WaveInput(const QString &f) :
  file_(f)
{
}

WaveInput::~WaveInput()
{
  close();
}

bool WaveInput::open()
{
  if (!file_.open(QFile::ReadOnly)) {
    return false;
  }

  if (file_.read(4) != "RIFF") {
    close();
    qCritical() << "No RIFF found";
    return false;
  }

  // Skip filesize bytes
  file_.seek(file_.pos() + 4);

  if (file_.read(4) != "WAVE") {
    close();
    qCritical() << "No WAVE found";
    return false;
  }

  // Find fmt_ section
  if (!find_str(&file_, "fmt ")) {
    close();
    qCritical() << "No fmt  found";
    return false;
  }

  // Skip fmt_ section size
  file_.seek(file_.pos()+4);

  // Create data stream for reading bytes into types
  QDataStream data_stream(&file_);
  data_stream.setByteOrder(QDataStream::LittleEndian);

  // Read data type
  uint16_t data_type;
  data_stream >> data_type;

  bool data_is_float;
  switch (data_type) {
  case 1: // PCM Integer
    data_is_float = false;
    break;
  case 3:
    data_is_float = true;
    break;
  default:
    // If it's neither float nor int, we can't work with this file
    close();
    qCritical() << "Invalid WAV type" << data_type;
    return false;
  }

  // Read number of channels
  uint16_t channel_count;
  data_stream >> channel_count;

  uint64_t channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(channel_count));

  int32_t sample_rate;
  data_stream >> sample_rate;

  // Skip bytes per second value and bytes per sample value
  file_.seek(file_.pos() + 6);

  uint16_t bits_per_sample;
  data_stream >> bits_per_sample;

  AudioParams::Format format;

  switch (bits_per_sample) {
  case 8:
    format = AudioParams::kFormatUnsigned8;
    break;
  case 16:
    format = AudioParams::kFormatSigned16;
    break;
  case 32:
    if (data_is_float) {
      format = AudioParams::kFormatFloat32;
    } else {
      format = AudioParams::kFormatSigned32;
    }
    break;
  case 64:
    if (data_is_float) {
      format = AudioParams::kFormatFloat64;
    } else {
      format = AudioParams::kFormatSigned64;
    }
    break;
  default:
    // We don't know this format...
    close();
    qCritical() << "Invalid format found" << bits_per_sample;
    return false;
  }

  // We're good to go!
  params_ = AudioParams(sample_rate, channel_layout, format);

  if (!find_str(&file_, "data")) {
    close();
    qCritical() << "No data tag found";
    return false;
  }

  data_stream >> data_size_;
  data_position_ = file_.pos();

  return true;
}

bool WaveInput::is_open() const
{
  return file_.isOpen();
}

QByteArray WaveInput::read(int length)
{
  if (!is_open()) {
    return QByteArray();
  }

  return file_.read(qMin(calculate_max_read(), static_cast<qint64>(length)));
}

QByteArray WaveInput::read(int offset, int length)
{
  if (!is_open()) {
    return QByteArray();
  }

  seek(offset);
  return file_.read(qMin(calculate_max_read(), static_cast<qint64>(length)));
}

qint64 WaveInput::read(int offset, char *buffer, int length)
{
  if (!is_open()) {
    return 0;
  }

  seek(offset);
  return file_.read(buffer, qMin(calculate_max_read(), static_cast<qint64>(length)));
}

bool WaveInput::seek(qint64 pos)
{
  return file_.seek(data_position_ + qMin(pos, static_cast<qint64>(data_size_)));
}

bool WaveInput::at_end() const
{
  return file_.pos() == (data_position_ + data_size_);
}

const AudioParams &WaveInput::params() const
{
  return params_;
}

void WaveInput::close()
{
  if (file_.isOpen()) {
    file_.close();
  }
}

const quint32 &WaveInput::data_length() const
{
  return data_size_;
}

int WaveInput::sample_count() const
{
  return params_.bytes_to_samples(static_cast<int>(data_size_));
}

bool WaveInput::find_str(QFile *f, const char *str)
{
  qint64 pos = f->pos();
  while (f->read(4) != str) {
    if (f->atEnd()) {
      return false;
    }

    pos++;
    f->seek(pos);
  }

  return true;
}

qint64 WaveInput::calculate_max_read() const
{
  return data_size_ - (file_.pos() - data_position_ );
}

}
